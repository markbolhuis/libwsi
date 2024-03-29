#include <assert.h>

#include <xcb/xcb.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "wsi/egl.h"
#include "wsi/window.h"

#include "utils.h"

#include "common_priv.h"
#include "platform_priv.h"
#include "window_priv.h"

WsiResult
wsiGetEGLDisplay(WsiPlatform platform, EGLDisplay *pDisplay)
{
    EGLAttrib attrib[] = {
        EGL_PLATFORM_XCB_SCREEN_EXT, platform->xcb_screen_id,
        EGL_NONE,
    };

    *pDisplay = eglGetPlatformDisplay(EGL_PLATFORM_XCB_EXT, platform->xcb_connection, attrib);
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
    struct wsi_platform *platform = window->platform;

    EGLint visualid = 0;
    EGLBoolean ok = eglGetConfigAttrib(dpy, config, EGL_NATIVE_VISUAL_ID, &visualid);
    if (ok == EGL_FALSE) {
        return WSI_ERROR_EGL;
    }

    window->xcb_colormap = xcb_generate_id(platform->xcb_connection);
    if (window->xcb_colormap == UINT32_MAX) {
        return WSI_ERROR_PLATFORM;
    }

    xcb_create_colormap_checked(
        platform->xcb_connection,
        XCB_COLORMAP_ALLOC_NONE,
        window->xcb_colormap,
        platform->xcb_screen->root,
        (xcb_visualid_t)visualid);

    xcb_change_window_attributes_checked(
        platform->xcb_connection,
        window->xcb_window,
        XCB_CW_COLORMAP,
        (const uint32_t[]){ window->xcb_colormap });

    *pSurface = eglCreatePlatformWindowSurface(dpy, config, &window->xcb_window, NULL);
    if (*pSurface == EGL_NO_SURFACE) {
        return WSI_ERROR_EGL;
    }

    return WSI_SUCCESS;
}

void
wsiDestroyWindowEGLSurface(WsiWindow window, EGLDisplay dpy, EGLSurface surface)
{
    struct wsi_platform *platform = window->platform;

    eglDestroySurface(dpy, surface);

    const uint32_t value_list[] = {
        platform->xcb_screen->default_colormap,
    };

    xcb_change_window_attributes(
        platform->xcb_connection,
        window->xcb_window,
        XCB_CW_COLORMAP,
        value_list);

    xcb_free_colormap(platform->xcb_connection, window->xcb_colormap);
    window->xcb_colormap = XCB_NONE;
}
