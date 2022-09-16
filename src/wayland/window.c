#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>

#include <wayland-client-protocol.h>
#include <xdg-shell-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>

#include "wsi/window.h"

#include "platform_priv.h"
#include "window_priv.h"

struct wsi_window_output {
    struct wl_list link;
    struct wl_output *wl_output;
};

static inline struct wsi_xdg_extent
wsi_extent_to_xdg(
    struct wsi_extent extent)
{
    assert(extent.width <= INT32_MAX);
    assert(extent.height <= INT32_MAX);

    struct wsi_xdg_extent xdg = {
        .width = (int32_t)extent.width,
        .height = (int32_t)extent.height,
    };

    return xdg;
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

    struct wsi_xdg_state pending = {0};

    uint32_t *state = NULL;
    wl_array_for_each(state, states) {
        switch (*state) {
            case XDG_TOPLEVEL_STATE_MAXIMIZED:
                pending.maximized = true;
                break;
            case XDG_TOPLEVEL_STATE_FULLSCREEN:
                pending.fullscreen = true;
                break;
            case XDG_TOPLEVEL_STATE_RESIZING:
                pending.resizing = true;
                break;
            case XDG_TOPLEVEL_STATE_ACTIVATED:
                pending.activated = true;
                break;
            case XDG_TOPLEVEL_STATE_TILED_LEFT:
                pending.tiled_left = true;
                break;
            case XDG_TOPLEVEL_STATE_TILED_RIGHT:
                pending.tiled_right = true;
                break;
            case XDG_TOPLEVEL_STATE_TILED_TOP:
                pending.tiled_top = true;
                break;
            case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
                pending.tiled_bottom = true;
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

    struct wsi_xdg_capabilities pending = {0};

    uint32_t *cap = NULL;
    wl_array_for_each(cap, capabilities) {
        switch (*cap) {
            case XDG_TOPLEVEL_WM_CAPABILITIES_WINDOW_MENU:
                pending.window_menu = true;
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE:
                pending.maximize;
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN:
                pending.fullscreen = true;
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_MINIMIZE:
                pending.minimize = true;
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

    // TODO: Handle this properly
    window->current = window->pending;
    memset(&window->pending, 0, sizeof(struct wsi_window_state));

    window->event_mask = WSI_XDG_EVENT_NONE;
    xdg_surface_ack_configure(xdg_surface, serial);
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
    struct wl_list *output_list = &window->output_list;

    bool found = false;
    struct wsi_window_output *window_output;
    wl_list_for_each(window_output, output_list, link) {
        if (window_output->wl_output == wl_output) {
            found = true;
            break;
        }
    }

    if (found) {
        return;
    }

    window_output = calloc(1, sizeof(struct wsi_window_output));
    if (!window_output) {
        return;
    }

    window_output->wl_output = wl_output;
    wl_list_insert(output_list, &window_output->link);
}

static void
wl_surface_leave(
    void *data,
    struct wl_surface *wl_surface,
    struct wl_output *wl_output)
{
    struct wsi_window *window = data;
    struct wl_list *output_list = &window->output_list;

    bool found = false;
    struct wsi_window_output *window_output;
    wl_list_for_each(window_output, output_list, link) {
        if (window_output->wl_output == wl_output) {
            found = true;
            break;
        }
    }

    if (!found) {
        return;
    }

    wl_list_remove(&window_output->link);
    free(window_output);
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
    window->user_extent = wsi_extent_to_xdg(pCreateInfo->extent);

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

    wl_surface_commit(window->wl_surface);

    wl_display_roundtrip(platform->wl_display);

    *pWindow = window;
    return WSI_SUCCESS;
}

void
wsiDestroyWindow(
    WsiPlatform platform,
    WsiWindow window)
{
    if (window->xdg_toplevel_decoration_v1) {
        zxdg_toplevel_decoration_v1_destroy(window->xdg_toplevel_decoration_v1);
    }

    xdg_toplevel_destroy(window->xdg_toplevel);
    xdg_surface_destroy(window->xdg_surface);

    wl_surface_destroy(window->wl_surface);

    wl_display_flush(platform->wl_display);

    free(window);
}

WsiResult
wsiGetWindowParent(
    WsiWindow window,
    WsiWindow *pParent)
{
    *pParent = window->parent;
    return WSI_SUCCESS;
}

WsiResult
wsiSetWindowParent(
    WsiWindow window,
    WsiWindow parent)
{
    struct xdg_toplevel *xdg_parent = NULL;
    if (parent != NULL) {
        xdg_parent = parent->xdg_toplevel;
    }
    xdg_toplevel_set_parent(window->xdg_toplevel, xdg_parent);
    window->parent = parent;
    return WSI_SUCCESS;
}

void
wsiGetWindowExtent(
    WsiWindow window,
    WsiExtent *pExtent)
{
    assert(window->current.extent.width > 0);
    assert(window->current.extent.height > 0);

    pExtent->width = (uint32_t)window->current.extent.width;
    pExtent->height = (uint32_t)window->current.extent.height;
}

WsiResult
wsiSetWindowTitle(
    WsiWindow window,
    const char *pTitle)
{
    xdg_toplevel_set_title(window->xdg_toplevel, pTitle);
    return WSI_SUCCESS;
}

void
wsiGetWindowFeatures(
    WsiWindow window,
    WsiWindowFeatures *pFeatures)
{
    pFeatures->move = true;
    pFeatures->resize = true;
    pFeatures->decoration = window->xdg_toplevel_decoration_v1 != NULL;
}

