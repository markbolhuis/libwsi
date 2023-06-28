#include <assert.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-egl-core.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "common_priv.h"
#include "platform_priv.h"
#include "window_priv.h"

#include "wsi/egl.h"

WsiResult
wsiGetEGLDisplay(WsiPlatform platform, EGLDisplay *pDisplay)
{
    *pDisplay = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, platform->wl_display, NULL);
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
    assert(window->wl_egl_window == NULL);

    WsiExtent extent = wsi_window_get_buffer_extent(window);

    window->wl_egl_window = wl_egl_window_create(
        window->wl_surface,
        extent.width,
        extent.height);
    if (window->wl_egl_window == NULL) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    *pSurface = eglCreatePlatformWindowSurface(dpy, config, window->wl_egl_window, NULL);
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
    assert(window->wl_egl_window != NULL);

    eglDestroySurface(dpy, surface);
    wl_egl_window_destroy(window->wl_egl_window);
    window->wl_egl_window = NULL;
    window->api = WSI_API_NONE;
}
