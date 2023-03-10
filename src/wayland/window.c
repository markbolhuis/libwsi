#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>

#include <wayland-client-protocol.h>
#include <wayland-egl-core.h>
#include <viewporter-client-protocol.h>
#include <fractional-scale-v1-client-protocol.h>
#include <xdg-shell-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>

#include "common_priv.h"
#include "platform_priv.h"
#include "output_priv.h"
#include "window_priv.h"

struct wsi_window_output {
    struct wl_list link;
    struct wl_output *wl_output;
};

WsiExtent
wsi_window_get_buffer_extent(struct wsi_window *window)
{
    WsiExtent extent = window->current.extent;
    extent.width *= window->current.scale;
    extent.height *= window->current.scale;
    if (window->wp_fractional_scale_v1) {
        extent.width = div_round(extent.width, 120);
        extent.height = div_round(extent.height, 120);
    }
    return extent;
}

static int32_t
wsi_window_calculate_output_max_scale(struct wsi_window *window)
{
    int32_t max_scale = 1;

    struct wsi_window_output *wo;
    wl_list_for_each(wo, &window->output_list, link) {
        struct wsi_output *output = wl_output_get_user_data(wo->wl_output);
        int32_t scale = wsi_output_get_scale(output);
        if (scale > max_scale) {
            max_scale = scale;
        }
    }

    return max_scale;
}

static struct wsi_window_output *
wsi_window_find_output(struct wsi_window *window, struct wl_output *wl_output)
{
    struct wsi_window_output *wo;
    wl_list_for_each(wo, &window->output_list, link) {
        if (wo->wl_output == wl_output) {
            return wo;
        }
    }

    return NULL;
}

static void
wsi_window_configure(struct wsi_window *window)
{
    uint32_t mask = window->event_mask;

    struct wsi_window_state *pending = &window->pending;
    struct wsi_window_state *current = &window->current;

    bool resized = false;
    if (mask & WSI_XDG_EVENT_CONFIGURE) {
        resized = !wsi_extent_equal(current->extent, pending->extent);
        if (resized) {
            current->extent = pending->extent;
        }

        current->state = pending->state;
    }

    if (mask & WSI_XDG_EVENT_BOUNDS) {
        current->bounds = pending->bounds;
    }

    if (mask & WSI_XDG_EVENT_WM_CAPABILITIES) {
        current->capabilities = pending->capabilities;
    }

    if (mask & WSI_XDG_EVENT_DECORATION) {
        current->decoration = pending->decoration;
    }

    bool rescaled = false;
    if (mask & WSI_XDG_EVENT_SCALE) {
        rescaled = pending->scale != current->scale;
        if (rescaled) {
            current->scale = pending->scale;
            resized = true;
        }
    }

    window->event_mask = WSI_XDG_EVENT_NONE;

    if (resized) {
        WsiExtent be = wsi_window_get_buffer_extent(window);

        if (window->api == WSI_API_EGL) {
            assert(window->wl_egl_window != NULL);
            wl_egl_window_resize(window->wl_egl_window, be.width, be.height, 0, 0);
        }

        if (window->wp_fractional_scale_v1) {
            assert(window->wp_viewport != NULL);

            wp_viewport_set_source(
                window->wp_viewport,
                wl_fixed_from_int(0),
                wl_fixed_from_int(0),
                wl_fixed_from_int(be.width),
                wl_fixed_from_int(be.height));
            wp_viewport_set_destination(
                window->wp_viewport,
                window->current.extent.width,
                window->current.extent.height);
        }

        WsiConfigureWindowEvent info = {
            .base.type = WSI_EVENT_TYPE_CONFIGURE_WINDOW,
            .base.flags = 0,
            .base.serial = window->serial,
            .base.time = 0,
            .window = window,
            .extent = be,
        };

        window->pfn_configure(window->user_data, &info);
    }

    uint32_t version = wl_surface_get_version(window->wl_surface);
    if (rescaled &&
        window->wp_fractional_scale_v1 == NULL &&
        version >= WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION)
    {
        wl_surface_set_buffer_scale(window->wl_surface, current->scale);
    }

    if (window->serial != 0) {
        xdg_surface_ack_configure(window->xdg_surface, window->serial);
        window->serial = 0;
    }

    window->configured = true;
}

static void
wsi_window_set_initial_state(struct wsi_window *window)
{
    window->event_mask |= WSI_XDG_EVENT_SCALE;
    if (window->wp_fractional_scale_v1) {
        window->pending.scale = 120;
    } else {
        window->pending.scale = 1;
    }

    if (xdg_toplevel_get_version(window->xdg_toplevel) <
        XDG_TOPLEVEL_WM_CAPABILITIES_SINCE_VERSION)
    {
        window->event_mask |= WSI_XDG_EVENT_WM_CAPABILITIES;
        window->pending.capabilities = WSI_XDG_CAPABILITIES_WINDOW_MENU
                                     | WSI_XDG_CAPABILITIES_MAXIMIZE
                                     | WSI_XDG_CAPABILITIES_FULLSCREEN
                                     | WSI_XDG_CAPABILITIES_MINIMIZE;
    }
}

static bool
wsi_window_add_output(struct wsi_window *window, struct wl_output *wl_output)
{
    struct wsi_window_output *wo = wsi_window_find_output(window, wl_output);
    if (wo) {
        return false;
    }

    wo = calloc(1, sizeof(*wo));
    if (!wo) {
        return false;
    }

    wo->wl_output = wl_output;
    wl_list_insert(&window->output_list, &wo->link);
    return true;
}

static bool
wsi_window_remove_output(struct wsi_window *window, struct wl_output *wl_output)
{
    struct wsi_window_output *wo = wsi_window_find_output(window, wl_output);
    if (!wo) {
        return false;
    }

    wl_list_remove(&wo->link);
    free(wo);
    return true;
}

void
wsi_window_handle_output_destroyed(struct wsi_window *w, struct wsi_output *o)
{
    // TODO: Some compositors like KDE send a wl_output.leave event before destroying
    //  the wl_output object. Others like Sway do not, so the window scale needs to be
    //  updated when the wl_output object is destroyed.
}

// region XDG Toplevel Decoration

static void
xdg_toplevel_decoration_v1_configure(
    void *data,
    struct zxdg_toplevel_decoration_v1 *xdg_toplevel_decoration,
    uint32_t mode)
{
    struct wsi_window *window = data;

    window->event_mask |= WSI_XDG_EVENT_DECORATION;
    window->pending.decoration = mode;
}

static const struct zxdg_toplevel_decoration_v1_listener xdg_toplevel_decoration_v1_listener = {
    .configure = xdg_toplevel_decoration_v1_configure,
};

// endregion

// region XDG Toplevel

static void
xdg_toplevel_configure(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    int32_t width,
    int32_t height,
    struct wl_array *states)
{
    struct wsi_window *window = data;

    if (width == 0) {
        width = window->user_extent.width;
    }
    if (height == 0) {
        height = window->user_extent.height;
    }

    window->event_mask |= WSI_XDG_EVENT_CONFIGURE;
    window->pending.extent.width = width;
    window->pending.extent.height = height;

    enum wsi_xdg_state pending = WSI_XDG_STATE_NONE;

    uint32_t *state = NULL;
    wl_array_for_each(state, states) {
        switch (*state) {
            case XDG_TOPLEVEL_STATE_MAXIMIZED:
                pending |= WSI_XDG_STATE_MAXIMIZED;
                break;
            case XDG_TOPLEVEL_STATE_FULLSCREEN:
                pending |= WSI_XDG_STATE_FULLSCREEN;
                break;
            case XDG_TOPLEVEL_STATE_RESIZING:
                pending |= WSI_XDG_STATE_RESIZING;
                break;
            case XDG_TOPLEVEL_STATE_ACTIVATED:
                pending |= WSI_XDG_STATE_ACTIVATED;
                break;
            case XDG_TOPLEVEL_STATE_TILED_LEFT:
                pending |= WSI_XDG_STATE_TILED_LEFT;
                break;
            case XDG_TOPLEVEL_STATE_TILED_RIGHT:
                pending |= WSI_XDG_STATE_TILED_RIGHT;
                break;
            case XDG_TOPLEVEL_STATE_TILED_TOP:
                pending |= WSI_XDG_STATE_TILED_TOP;
                break;
            case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
                pending |= WSI_XDG_STATE_TILED_BOTTOM;
                break;
        }
    }

    window->pending.state = pending;
}

static void
xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
    struct wsi_window *window = data;

    WsiCloseWindowEvent info = {
        .base.type = WSI_EVENT_TYPE_CLOSE_WINDOW,
        .base.flags = 0,
        .base.serial = 0,
        .base.time = 0,
        .window = window,
    };

    window->pfn_close(window->user_data, &info);
}

static void
xdg_toplevel_configure_bounds(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    int32_t max_width,
    int32_t max_height)
{
    struct wsi_window *window = data;

    window->event_mask |= WSI_XDG_EVENT_BOUNDS;
    window->pending.bounds.width = max_width;
    window->pending.bounds.height = max_height;
}

static void
xdg_toplevel_wm_capabilities(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    struct wl_array *capabilities)
{
    struct wsi_window *window = data;

    enum wsi_xdg_capabilities pending = WSI_XDG_CAPABILITIES_NONE;

    uint32_t *cap = NULL;
    wl_array_for_each(cap, capabilities) {
        switch (*cap) {
            case XDG_TOPLEVEL_WM_CAPABILITIES_WINDOW_MENU:
                pending |= WSI_XDG_CAPABILITIES_WINDOW_MENU;
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE:
                pending |= WSI_XDG_CAPABILITIES_MAXIMIZE;
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN:
                pending |= WSI_XDG_CAPABILITIES_FULLSCREEN;
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_MINIMIZE:
                pending |= WSI_XDG_CAPABILITIES_MINIMIZE;
                break;
        }
    }

    window->event_mask |= WSI_XDG_EVENT_WM_CAPABILITIES;
    window->pending.capabilities = pending;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure        = xdg_toplevel_configure,
    .close            = xdg_toplevel_close,
    .configure_bounds = xdg_toplevel_configure_bounds,
    .wm_capabilities  = xdg_toplevel_wm_capabilities,
};

// endregion

// region XDG Surface

static void
xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
    struct wsi_window *window = data;
    window->serial = serial;
    wsi_window_configure(window);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

// endregion

// region WP Fractional Scale

static void
wp_fractional_scale_v1_preferred_scale(
    void *data,
    struct wp_fractional_scale_v1 *wp_fractional_scale_v1,
    uint32_t scale)
{
    struct wsi_window *window = data;

    window->event_mask |= WSI_XDG_EVENT_SCALE;
    window->pending.scale = (int32_t)scale;

    if (window->configured) {
        wsi_window_configure(window);
    }
}

static const struct wp_fractional_scale_v1_listener wp_fractional_scale_listener = {
    .preferred_scale = wp_fractional_scale_v1_preferred_scale,
};

// endregion

// region WL Surface

static void
wl_surface_enter(void *data, struct wl_surface *wl_surface, struct wl_output *wl_output)
{
    struct wsi_window *window = data;

    uint32_t version = wl_surface_get_version(wl_surface);
    bool added = wsi_window_add_output(window, wl_output);
    if (!added ||
        version < WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION ||
        window->wp_fractional_scale_v1)
    {
        return;
    }

    window->event_mask |= WSI_XDG_EVENT_SCALE;
    window->pending.scale = wsi_window_calculate_output_max_scale(window);

    if (window->configured) {
        wsi_window_configure(window);
    }
}

static void
wl_surface_leave(void *data, struct wl_surface *wl_surface, struct wl_output *wl_output)
{
    struct wsi_window *window = data;

    uint32_t version = wl_surface_get_version(wl_surface);
    bool removed = wsi_window_remove_output(window, wl_output);
    if (!removed ||
        version < WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION ||
        window->wp_fractional_scale_v1)
    {
        return;
    }

    window->event_mask |= WSI_XDG_EVENT_SCALE;
    window->pending.scale = wsi_window_calculate_output_max_scale(window);

    if (window->configured) {
        wsi_window_configure(window);
    }
}

static const struct wl_surface_listener wl_surface_listener = {
    .enter = wl_surface_enter,
    .leave = wl_surface_leave,
};

// endregion

static WsiResult
wsi_window_init(
    struct wsi_platform *platform,
    const WsiWindowCreateInfo *info,
    struct wsi_window *window)
{
    window->platform = platform;
    window->api = WSI_API_NONE;
    window->user_extent = info->extent;
    window->user_data = info->pUserData;
    window->pfn_close = info->pfnCloseWindow;
    window->pfn_configure = info->pfnConfigureWindow;

    wl_list_init(&window->output_list);

    window->wl_surface = wl_compositor_create_surface(platform->wl_compositor);
    wl_surface_add_listener(window->wl_surface, &wl_surface_listener, window);

    if (platform->wp_viewporter) {
        window->wp_viewport = wp_viewporter_get_viewport(
            platform->wp_viewporter,
            window->wl_surface);

        if (platform->wp_fractional_scale_manager_v1) {
            window->wp_fractional_scale_v1 = wp_fractional_scale_manager_v1_get_fractional_scale(
                platform->wp_fractional_scale_manager_v1,
                window->wl_surface);
            wp_fractional_scale_v1_add_listener(
                window->wp_fractional_scale_v1,
                &wp_fractional_scale_listener,
                window);
        }
    }

    window->xdg_surface = xdg_wm_base_get_xdg_surface(platform->xdg_wm_base, window->wl_surface);
    xdg_surface_add_listener(window->xdg_surface, &xdg_surface_listener, window);

    window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
    xdg_toplevel_add_listener(window->xdg_toplevel, &xdg_toplevel_listener, window);

    if (platform->xdg_decoration_manager_v1) {
        window->xdg_toplevel_decoration_v1 = zxdg_decoration_manager_v1_get_toplevel_decoration(
            platform->xdg_decoration_manager_v1,
            window->xdg_toplevel);
        zxdg_toplevel_decoration_v1_add_listener(
            window->xdg_toplevel_decoration_v1,
            &xdg_toplevel_decoration_v1_listener,
            window);
    }

    xdg_toplevel_set_title(window->xdg_toplevel, info->pTitle);

    if (info->parent != NULL) {
        window->parent = info->parent;
        xdg_toplevel_set_parent(window->xdg_toplevel, window->parent->xdg_toplevel);
    }

    wsi_window_set_initial_state(window);

    wl_surface_commit(window->wl_surface);

    int ret = 0;
    while(!window->configured && ret >= 0) {
        ret = wl_display_dispatch(platform->wl_display);
    }

    wl_list_insert(&platform->window_list, &window->link);
    return WSI_SUCCESS;
}

static void
wsi_window_uninit(struct wsi_window *window)
{
    wl_list_remove(&window->link);

    struct wsi_window_output *wo, *wm_tmp;
    wl_list_for_each_safe(wo, wm_tmp, &window->output_list, link) {
        wl_list_remove(&wo->link);
        free(wo);
    }

    if (window->xdg_toplevel_decoration_v1) {
        zxdg_toplevel_decoration_v1_destroy(window->xdg_toplevel_decoration_v1);
    }

    xdg_toplevel_destroy(window->xdg_toplevel);
    xdg_surface_destroy(window->xdg_surface);

    if (window->wp_fractional_scale_v1) {
        wp_fractional_scale_v1_destroy(window->wp_fractional_scale_v1);
    }

    if (window->wp_viewport) {
        wp_viewport_destroy(window->wp_viewport);
    }

    wl_surface_destroy(window->wl_surface);
}

WsiResult
wsiCreateWindow(
    WsiPlatform platform,
    const WsiWindowCreateInfo *pCreateInfo,
    WsiWindow *pWindow)
{
    assert(pCreateInfo->sType == WSI_STRUCTURE_TYPE_WINDOW_CREATE_INFO);

    if (platform->xdg_wm_base == NULL) {
        return WSI_ERROR_UNSUPPORTED;
    }

    struct wsi_window *window = calloc(1, sizeof(struct wsi_window));
    if (!window) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    WsiResult result = wsi_window_init(platform, pCreateInfo, window);
    if (result != WSI_SUCCESS) {
        free(window);
        return result;
    }

    *pWindow = window;
    return WSI_SUCCESS;
}

void
wsiDestroyWindow(WsiWindow window)
{
    wsi_window_uninit(window);
    free(window);
}

WsiResult
wsiSetWindowParent(WsiWindow window, WsiWindow parent)
{
    struct xdg_toplevel *xdg_parent = NULL;
    if (parent != NULL) {
        xdg_parent = parent->xdg_toplevel;
    }
    xdg_toplevel_set_parent(window->xdg_toplevel, xdg_parent);
    window->parent = parent;
    return WSI_SUCCESS;
}

WsiResult
wsiSetWindowTitle(WsiWindow window, const char *pTitle)
{
    if (pTitle == NULL) {
        pTitle = "";
    }
    xdg_toplevel_set_title(window->xdg_toplevel, pTitle);
    return WSI_SUCCESS;
}
