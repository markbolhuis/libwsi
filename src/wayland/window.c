#include <stdlib.h>
#include <assert.h>

#include <wayland-client-protocol.h>
#include <xdg-shell-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>

#include "wsi/window.h"

#include "platform_priv.h"
#include "window_priv.h"

// region XDG Toplevel Decoration

static void
xdg_toplevel_decoration_v1_configure(
    void *data,
    struct zxdg_toplevel_decoration_v1 *xdg_toplevel_decoration,
    uint32_t name)
{

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
    assert(window->xdg_toplevel == xdg_toplevel);

    int32_t user_width = window->user_extent.width;
    int32_t user_height = window->user_extent.height;

    if (user_width == 0) {
        user_width = 640;
    }
    if (user_height == 0) {
        user_height = 360;
    }

    if (width == 0 && height == 0) {
        width = user_width;
        height = user_height;
    }
    else if (width == 0) {
        width = (int32_t)(((int64_t)height * (int64_t)user_width) / (int64_t)user_height);
    }
    else if (height == 0) {
        height = (int32_t)(((int64_t)width * (int64_t)user_height) / (int64_t)user_width);
    }

    window->extent.width = width;
    window->extent.height = height;
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

}

static void
xdg_toplevel_wm_capabilities(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    struct wl_array *capabilities)
{

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
    assert(window->xdg_surface == xdg_surface);
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

}

static void
wl_surface_leave(
    void *data,
    struct wl_surface *wl_surface,
    struct wl_output *wl_output)
{

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

    window->platform = platform;

    window->user_extent.width = (int32_t)pCreateInfo->width;
    window->user_extent.height = (int32_t)pCreateInfo->height;

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
    uint32_t *width,
    uint32_t *height)
{
    assert(window->extent.width > 0);
    assert(window->extent.height > 0);

    *width = (uint32_t)window->extent.width;
    *height = (uint32_t)window->extent.height;
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

