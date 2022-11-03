#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>

#include <wayland-client-protocol.h>
#include <wayland-egl-core.h>
#include <xdg-shell-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>

#include "wsi/window.h"

#include "common_priv.h"
#include "platform_priv.h"
#include "window_priv.h"
#include "output_priv.h"

struct wsi_window_output {
    struct wl_list link;
    struct wl_output *wl_output;
};

struct wsi_extent
wsi_window_get_buffer_extent(struct wsi_window *window)
{
    struct wsi_extent extent = window->current.extent;
    extent.width *= window->current.scale;
    extent.height *= window->current.scale;
    return extent;
}

static int32_t
wsi_window_get_max_scale(struct wsi_window *window)
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
    // TODO: This is a bit basic, but works for now.

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

    window->event_mask = WSI_XDG_EVENT_NONE;

    bool rescaled = pending->scale != current->scale;
    if (rescaled) {
        current->scale = pending->scale;
        resized = true;
    }

    if (resized) {
        struct wsi_extent be = wsi_window_get_buffer_extent(window);

        // TODO: Decide how best to handle Vulkan or EGL windows.
        if (window->api == WSI_API_EGL) {
            assert(window->wl_egl_window != NULL);
            wl_egl_window_resize(
                window->wl_egl_window,
                be.width,
                be.height,
                0, 0);
        }

        window->pfn_configure(window->user_data, be);
    }

    uint32_t version = wl_surface_get_version(window->wl_surface);
    if (rescaled && version >= WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION) {
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
    window->pending.scale = 1;

    if (xdg_toplevel_get_version(window->xdg_toplevel) <
        XDG_TOPLEVEL_WM_CAPABILITIES_SINCE_VERSION)
    {
        window->event_mask |= WSI_XDG_EVENT_WM_CAPABILITIES;

        window->pending.capabilities = WSI_XDG_CAPABILITY_WINDOW_MENU
                                     | WSI_XDG_CAPABILITY_MAXIMIZE
                                     | WSI_XDG_CAPABILITY_FULLSCREEN
                                     | WSI_XDG_CAPABILITY_MINIMIZE;
    }
}

void
wsi_window_handle_output_destroyed(struct wsi_window *w, struct wsi_output *o)
{
    struct wsi_window_output *wo = wsi_window_find_output(w, o->wl_output);
    if (!wo) {
        return;
    }

    wl_list_remove(&wo->link);
    free(wo);

    if (wl_surface_get_version(w->wl_surface) >=
        WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION)
    {
        w->pending.scale = wsi_window_get_max_scale(w);
        wsi_window_configure(w);
    }
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
xdg_toplevel_close(
    void *data,
    struct xdg_toplevel *xdg_toplevel)
{
    struct wsi_window *window = data;
    window->pfn_close(window->user_data);
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
                pending |= WSI_XDG_CAPABILITY_WINDOW_MENU;
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE:
                pending |= WSI_XDG_CAPABILITY_MAXIMIZE;
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN:
                pending |= WSI_XDG_CAPABILITY_FULLSCREEN;
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_MINIMIZE:
                pending |= WSI_XDG_CAPABILITY_MINIMIZE;
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
xdg_surface_configure(
    void *data,
    struct xdg_surface *xdg_surface,
    uint32_t serial)
{
    struct wsi_window *window = data;
    window->serial = serial;
    wsi_window_configure(window);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

// endregion

// region WL Surface

static void
wl_surface_enter(
    void *data,
    struct wl_surface *wl_surface,
    struct wl_output *wl_output)
{
    struct wsi_window *window = data;

    struct wsi_window_output *wo = wsi_window_find_output(window, wl_output);
    if (wo) {
        return;
    }

    wo = calloc(1, sizeof(struct wsi_window_output));
    if (!wo) {
        return;
    }

    wo->wl_output = wl_output;
    wl_list_insert(&window->output_list, &wo->link);

    if (wl_surface_get_version(wl_surface) >=
        WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION)
    {
        window->pending.scale = wsi_window_get_max_scale(window);
        wsi_window_configure(window);
    }
}

static void
wl_surface_leave(
    void *data,
    struct wl_surface *wl_surface,
    struct wl_output *wl_output)
{
    struct wsi_window *window = data;

    struct wsi_window_output *wo = wsi_window_find_output(window, wl_output);
    if (!wo) {
        return;
    }

    wl_list_remove(&wo->link);
    free(wo);

    if (wl_surface_get_version(wl_surface) <
        WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION)
    {
        window->pending.scale = wsi_window_get_max_scale(window);
        wsi_window_configure(window);
    }
}

static const struct wl_surface_listener wl_surface_listener = {
    .enter = wl_surface_enter,
    .leave = wl_surface_leave,
};

// endregion

WsiResult
wsiCreateWindow(
    WsiPlatform platform,
    const WsiWindowCreateInfo *pCreateInfo,
    WsiWindow *pWindow)
{
    if (platform->xdg_wm_base == NULL) {
        return WSI_ERROR_UNSUPPORTED;
    }

    struct wsi_window *window = calloc(1, sizeof(struct wsi_window));
    if (window == NULL) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    wl_list_init(&window->output_list);

    window->platform = platform;
    window->api = WSI_API_NONE;
    window->user_extent = pCreateInfo->extent;
    window->user_data = pCreateInfo->pUserData;
    window->pfn_close = pCreateInfo->pfnClose;
    window->pfn_configure = pCreateInfo->pfnConfigure;

    window->wl_surface = wl_compositor_create_surface(platform->wl_compositor);
    wl_surface_add_listener(window->wl_surface, &wl_surface_listener, window);

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

    xdg_toplevel_set_title(window->xdg_toplevel, pCreateInfo->pTitle);

    if (pCreateInfo->parent != NULL) {
        window->parent = pCreateInfo->parent;
        xdg_toplevel_set_parent(window->xdg_toplevel, window->parent->xdg_toplevel);
    }

    wsi_window_set_initial_state(window);

    wl_surface_commit(window->wl_surface);

    int ret = 0;
    while(!window->configured && ret >= 0) {
        ret = wl_display_dispatch(platform->wl_display);
    }

    wl_list_insert(&platform->window_list, &window->link);
    *pWindow = window;
    return WSI_SUCCESS;
}

void
wsiDestroyWindow(WsiWindow window)
{
    struct wsi_platform *platform = window->platform;

    wl_list_remove(&window->link);

    if (window->xdg_toplevel_decoration_v1) {
        zxdg_toplevel_decoration_v1_destroy(window->xdg_toplevel_decoration_v1);
    }

    xdg_toplevel_destroy(window->xdg_toplevel);
    xdg_surface_destroy(window->xdg_surface);

    wl_surface_destroy(window->wl_surface);

    wl_display_roundtrip(platform->wl_display);

    struct wsi_window_output *wo, *wm_tmp;
    wl_list_for_each_safe(wo, wm_tmp, &window->output_list, link) {
        wl_list_remove(&wo->link);
        free(wo);
    }

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
