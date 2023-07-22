#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>

#include <wayland-client-protocol.h>
#include <wayland-egl-core.h>
#include <viewporter-client-protocol.h>
#include <fractional-scale-v1-client-protocol.h>
#include <content-type-v1-client-protocol.h>
#include <idle-inhibit-unstable-v1-client-protocol.h>
#include <xdg-shell-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>

#include "common_priv.h"
#include "platform_priv.h"
#include "output_priv.h"
#include "window_priv.h"

static inline bool
wsi_is_transform_vertical(int32_t transform)
{
    return (transform & WL_OUTPUT_TRANSFORM_90) == WL_OUTPUT_TRANSFORM_90;
}

static inline bool
wsi_is_transform_a_resize(int32_t before, int32_t after)
{
    return ((before ^ after) & WL_OUTPUT_TRANSFORM_90) == WL_OUTPUT_TRANSFORM_90;
}

WsiExtent
wsi_window_get_geometry_extent(struct wsi_window *window)
{
    return window->current.extent;
}

WsiExtent
wsi_window_get_surface_extent(struct wsi_window *window)
{
    // Until CSD is implemented surface extent is the same as geometry
    return wsi_window_get_geometry_extent(window);
}

WsiExtent
wsi_window_get_buffer_extent(struct wsi_window *window)
{
    WsiExtent extent = wsi_window_get_surface_extent(window);
    if (wsi_is_transform_vertical(window->current.transform)) {
        int32_t tmp = extent.width;
        extent.width = extent.height;
        extent.height = tmp;
    }
    extent.width *= window->current.scale;
    extent.height *= window->current.scale;
    if (window->wp_fractional_scale_v1) {
        extent.width = wsi_div_round(extent.width, 120);
        extent.height = wsi_div_round(extent.height, 120);
    }
    return extent;
}

static void
wsi_window_configure(struct wsi_window *window, uint32_t serial)
{
    enum wsi_xdg_event mask = window->event_mask;

    struct wsi_window_state *pending = &window->pending;
    struct wsi_window_state *current = &window->current;

    bool resized = false;
    if (mask & WSI_XDG_EVENT_EXTENT) {
        if (pending->extent.width == 0) {
            pending->extent.width = current->extent.width;
        }
        if (pending->extent.height == 0) {
            pending->extent.height = current->extent.height;
        }

        resized = !wsi_extent_equal(pending->extent, current->extent);
        current->extent = pending->extent;
    }

    if (mask & WSI_XDG_EVENT_STATE) {
        current->state = pending->state;
    }

    if (mask & WSI_XDG_EVENT_BOUNDS) {
        current->bounds = pending->bounds;
    }

    if (mask & WSI_XDG_EVENT_CAPABILITIES) {
        current->capabilities = pending->capabilities;
    }

    if (mask & WSI_XDG_EVENT_DECORATION) {
        current->decoration = pending->decoration;
    }

    bool rescaled = false;
    if (mask & WSI_XDG_EVENT_SCALE) {
        current->scale = pending->scale;
        rescaled = true;
        resized = true;
    }

    bool transformed = false;
    if (mask & WSI_XDG_EVENT_TRANSFORM) {
        resized |= wsi_is_transform_a_resize(current->transform, pending->transform);
        current->transform = pending->transform;
        transformed = true;
    }

    window->event_mask = WSI_XDG_EVENT_NONE;

    if (resized || !window->configured) {
        WsiExtent be = wsi_window_get_buffer_extent(window);
        WsiExtent se = wsi_window_get_surface_extent(window);

        if (window->wl_egl_window != NULL) {
            wl_egl_window_resize(window->wl_egl_window, be.width, be.height, 0, 0);
        }

        if (window->wp_fractional_scale_v1) {
            assert(window->wp_viewport != NULL);
            wp_viewport_set_destination(window->wp_viewport, se.width, se.height);
        }

        WsiConfigureWindowEvent info = {
            .base.type = WSI_EVENT_TYPE_CONFIGURE_WINDOW,
            .base.flags = 0,
            .base.serial = serial,
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

    if (transformed && version >= WL_SURFACE_SET_BUFFER_TRANSFORM_SINCE_VERSION) {
        wl_surface_set_buffer_transform(window->wl_surface, current->transform);
    }

}

// region Output

static void
wsi_window_update_output(struct wsi_window *window, struct wl_output *wl_output, bool added)
{
    struct wsi_output *output = wl_output_get_user_data(wl_output);
    uint32_t version = wl_surface_get_version(window->wl_surface);
    struct wl_list *list = &window->output_list;

    bool updated = added
                 ? wsi_output_ref_list_add(list, output)
                 : wsi_output_ref_list_remove(list, output);

    if (!updated||
        version < WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION ||
#ifdef WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION
        version >= WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION ||
#endif
        window->wp_fractional_scale_v1)
    {
        return;
    }

    window->event_mask &= ~WSI_XDG_EVENT_SCALE;

    int32_t scale = wsi_output_ref_list_get_max_scale(list);
    if (window->current.scale != scale) {
        window->event_mask |= WSI_XDG_EVENT_SCALE;
        window->pending.scale = scale;

        if (window->configured) {
            wsi_window_configure(window, 0);
        }
    }
}

void
wsi_window_handle_output_destroyed(struct wsi_window *w, struct wsi_output *o)
{
    // TODO: Some compositors like KDE send a wl_output.leave event before destroying
    //  the wl_output object. Others like Sway do not, so the window scale needs to be
    //  updated when the wl_output object is destroyed.
}

// endregion

// region XDG Toplevel Decoration

static void
xdg_toplevel_decoration_v1_configure(
    void *data,
    struct zxdg_toplevel_decoration_v1 *xdg_toplevel_decoration,
    uint32_t mode)
{
    struct wsi_window *window = data;

    window->event_mask &= ~WSI_XDG_EVENT_DECORATION;

    if (window->current.decoration != mode) {
        window->event_mask |= WSI_XDG_EVENT_DECORATION;
        window->pending.decoration = mode;
    }
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
#ifdef XDG_TOPLEVEL_STATE_SUSPENDED_SINCE_VERSION
            case XDG_TOPLEVEL_STATE_SUSPENDED:
                pending |= WSI_XDG_STATE_SUSPENDED;
                break;
#endif
        }
    }

    window->event_mask &= ~(WSI_XDG_EVENT_EXTENT | WSI_XDG_EVENT_STATE);

    if (window->current.extent.width != width ||
        window->current.extent.height != height)
    {
        window->event_mask |= WSI_XDG_EVENT_EXTENT;
        window->pending.extent.width = width;
        window->pending.extent.height = height;
    }

    if (window->current.state != pending) {
        window->event_mask |= WSI_XDG_EVENT_STATE;
        window->pending.state = pending;
    }
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

    window->event_mask &= ~WSI_XDG_EVENT_BOUNDS;

    if (window->current.bounds.width != max_width ||
        window->current.bounds.height != max_height)
    {
        window->event_mask |= WSI_XDG_EVENT_BOUNDS;
        window->pending.bounds.width = max_width;
        window->pending.bounds.height = max_height;
    }
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

    window->event_mask &= ~WSI_XDG_EVENT_CAPABILITIES;

    if (window->current.capabilities != pending) {
        window->event_mask |= WSI_XDG_EVENT_CAPABILITIES;
        window->pending.capabilities = pending;
    }
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

    if (window->event_mask != WSI_XDG_EVENT_NONE || !window->configured) {
        wsi_window_configure(window, serial);
    }

    xdg_surface_ack_configure(window->xdg_surface, serial);
    window->configured = true;
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

    window->event_mask &= ~WSI_XDG_EVENT_SCALE;

    if (window->current.scale != (int32_t)scale) {
        window->event_mask |= WSI_XDG_EVENT_SCALE;
        window->pending.scale = (int32_t)scale;

        if (window->configured) {
            wsi_window_configure(window, 0);
        }
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
    wsi_window_update_output(window, wl_output, true);
}

static void
wl_surface_leave(void *data, struct wl_surface *wl_surface, struct wl_output *wl_output)
{
    struct wsi_window *window = data;
    wsi_window_update_output(window, wl_output, false);
}

#ifdef WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION
static void
wl_surface_preferred_buffer_scale(void *data, struct wl_surface *wl_surface, int32_t factor)
{
    struct wsi_window *window = data;

    if (window->wp_fractional_scale_v1 != NULL) {
        return;
    }

    window->event_mask &= ~WSI_XDG_EVENT_SCALE;

    if (window->current.scale != factor) {
        window->event_mask |= WSI_XDG_EVENT_SCALE;
        window->pending.scale = factor;

        if (window->configured) {
            wsi_window_configure(window, 0);
        }
    }
}
#endif

#ifdef WL_SURFACE_PREFERRED_BUFFER_TRANSFORM_SINCE_VERSION
static void
wl_surface_preferred_buffer_transform(void *data, struct wl_surface *wl_surface, uint32_t transform)
{
    struct wsi_window *window = data;

    window->event_mask &= ~WSI_XDG_EVENT_TRANSFORM;

    if (window->current.transform != (int32_t)transform) {
        window->event_mask |= WSI_XDG_EVENT_TRANSFORM;
        window->pending.transform = (int32_t)transform;

        if (window->configured) {
            wsi_window_configure(window, 0);
        }
    }
}
#endif

static const struct wl_surface_listener wl_surface_listener = {
    .enter                      = wl_surface_enter,
    .leave                      = wl_surface_leave,
#ifdef WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION
    .preferred_buffer_scale     = wl_surface_preferred_buffer_scale,
#endif
#ifdef WL_SURFACE_PREFERRED_BUFFER_TRANSFORM_SINCE_VERSION
    .preferred_buffer_transform = wl_surface_preferred_buffer_transform,
#endif
};

// endregion

void
wsi_window_inhibit_idling(struct wsi_window *window, bool enable)
{
    if (enable) {
        assert(window->wp_idle_inhibitor_v1 == NULL);

        window->wp_idle_inhibitor_v1 = zwp_idle_inhibit_manager_v1_create_inhibitor(
            window->platform->wp_idle_inhibit_manager_v1,
            window->wl_surface);
    } else {
        assert(window->wp_idle_inhibitor_v1 != NULL);

        zwp_idle_inhibitor_v1_destroy(window->wp_idle_inhibitor_v1);
        window->wp_idle_inhibitor_v1 = NULL;
    }
}

void
wsi_window_set_content_type(struct wsi_window *window, uint32_t type)
{
    assert(window->wp_content_type_v1 != NULL);
    wp_content_type_v1_set_content_type(window->wp_content_type_v1, type);
}

static void
wsi_window_init_state(struct wsi_window *window, const WsiWindowCreateInfo *info)
{
    if (window->wp_fractional_scale_v1) {
        window->current.scale = 120;
    } else {
        window->current.scale = 1;
    }
    window->current.transform = WL_OUTPUT_TRANSFORM_NORMAL;
    window->current.extent = info->extent;
    window->current.state = WSI_XDG_STATE_NONE;
    window->current.bounds.width = 0;
    window->current.bounds.height = 0;
    window->current.capabilities = WSI_XDG_CAPABILITIES_NONE;
    window->current.decoration = ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE;

    if (xdg_toplevel_get_version(window->xdg_toplevel) <
        XDG_TOPLEVEL_WM_CAPABILITIES_SINCE_VERSION)
    {
        window->event_mask |= WSI_XDG_EVENT_CAPABILITIES;
        window->pending.capabilities = WSI_XDG_CAPABILITIES_WINDOW_MENU
                                     | WSI_XDG_CAPABILITIES_MAXIMIZE
                                     | WSI_XDG_CAPABILITIES_FULLSCREEN
                                     | WSI_XDG_CAPABILITIES_MINIMIZE;
    }

    if (info->pTitle != NULL) {
        xdg_toplevel_set_title(window->xdg_toplevel, info->pTitle);
    }

    if (info->parent != NULL) {
        xdg_toplevel_set_parent(window->xdg_toplevel, info->parent->xdg_toplevel);
    }
}

static WsiResult
wsi_window_init(
    struct wsi_platform *platform,
    const WsiWindowCreateInfo *info,
    struct wsi_window *window)
{
    window->platform = platform;
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

    if (platform->wp_content_type_manager_v1) {
        window->wp_content_type_v1 = wp_content_type_manager_v1_get_surface_content_type(
            platform->wp_content_type_manager_v1,
            window->wl_surface);
    }

    wsi_window_init_state(window, info);
    wl_surface_commit(window->wl_surface);

    wl_list_insert(&platform->window_list, &window->link);
    return WSI_SUCCESS;
}

static void
wsi_window_uninit(struct wsi_window *window)
{
    wl_list_remove(&window->link);

    struct wsi_output_ref *wo, *wm_tmp;
    wl_list_for_each_safe(wo, wm_tmp, &window->output_list, link) {
        wl_list_remove(&wo->link);
        free(wo);
    }

    if (window->wp_content_type_v1) {
        wp_content_type_v1_destroy(window->wp_content_type_v1);
    }

    if (window->wp_idle_inhibitor_v1) {
        zwp_idle_inhibitor_v1_destroy(window->wp_idle_inhibitor_v1);
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

// region Public API

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

// endregion
