#include <assert.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-egl-core.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "wsi/window.h"
#include "wsi/egl.h"

#include "platform_priv.h"
#include "window_priv.h"

WsiResult
wsiGetEGLDisplay(WsiPlatform platform, EGLDisplay *pDisplay)
{
    EGLAttrib attrib[] = {
        EGL_NONE,
    };

    *pDisplay = eglGetPlatformDisplay(
        EGL_PLATFORM_WAYLAND_KHR,
        platform->wl_display,
        attrib);
    if (*pDisplay == EGL_NO_DISPLAY) {
        return WSI_ERROR_EGL;
    }

    return WSI_SUCCESS;
}

WsiResult
wsiCreateWindowEGLSurface(
    WsiWindow window,
    EGLDisplay dpy,
    EGLConfig config,
    EGLSurface *pSurface)
{
    if (window->api != WSI_API_NONE) {
        return WSI_ERROR_WINDOW_IN_USE;
    }

    WsiExtent extent = wsi_window_get_buffer_extent(window);

    window->wl_egl_window = wl_egl_window_create(
        window->wl_surface,
        extent.width,
        extent.height);
    if (window->wl_egl_window == NULL) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    if (wl_surface_get_version(window->wl_surface) >=
        WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION)
    {
        wl_surface_set_buffer_scale(window->wl_surface, window->current.scale);
    }

    EGLint alpha;
    eglGetConfigAttrib(dpy, config, EGL_ALPHA_SIZE, &alpha);
    if (alpha == 0) {
        struct wl_region *wl_region = wl_compositor_create_region(
            window->platform->wl_compositor);
        wl_region_add(wl_region, 0, 0, INT32_MAX, INT32_MAX);
        wl_surface_set_opaque_region(window->wl_surface, wl_region);
        wl_region_destroy(wl_region);
    }

    EGLAttrib attrs[] = {
        EGL_NONE,
    };

    *pSurface = eglCreatePlatformWindowSurface(
        dpy,
        config,
        window->wl_egl_window,
        attrs);
    if (*pSurface == EGL_NO_SURFACE) {
        return WSI_ERROR_EGL;
    }

    window->api = WSI_API_EGL;
    return WSI_SUCCESS;
}

void
wsiDestroyWindowEGLSurface(WsiWindow window, EGLDisplay dpy, EGLSurface surface)
{
    assert(window->api == WSI_API_EGL);

    eglDestroySurface(dpy, surface);
    wl_egl_window_destroy(window->wl_egl_window);
    wl_surface_set_opaque_region(window->wl_surface, NULL);
    window->api = WSI_API_NONE;
}
