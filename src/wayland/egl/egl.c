#include <wayland-util.h>
#include <wayland-egl-core.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "wsi/egl/egl.h"

#include "../platform_priv.h"
#include "../window_priv.h"
#include "egl_priv.h"

void
wsi_window_egl_configure(
    struct wsi_window *window,
    struct wsi_wl_extent extent)
{
    if (window->wl_egl_window) {
        wl_egl_window_resize(
            window->wl_egl_window,
            window->current.extent.width,
            window->current.extent.height,
            0,
            0);
    }
    else {
        window->wl_egl_window = wl_egl_window_create(
            window->wl_surface,
            window->current.extent.width,
            window->current.extent.height);
    }
}

WsiResult
wsiGetEglDisplay(
    WsiPlatform platform,
    EGLDisplay *pDisplay)
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
wsiCreateWindowEglSurface(
    WsiWindow window,
    EGLDisplay dpy,
    EGLConfig config,
    EGLSurface *pSurface)
{
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

    return WSI_SUCCESS;
}


