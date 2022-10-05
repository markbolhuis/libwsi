#include <assert.h>

#include <xcb/xcb.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "wsi/egl/egl.h"
#include "wsi/window.h"

#include "utils.h"

#include "../common_priv.h"
#include "../platform_priv.h"
#include "../window_priv.h"

WsiResult
wsiGetEGLDisplay(
    WsiPlatform platform,
    EGLDisplay *pDisplay)
{
    EGLAttrib attrib[] = {
        EGL_PLATFORM_XCB_SCREEN_EXT, platform->xcb_screen_id,
        EGL_NONE,
    };

    *pDisplay = eglGetPlatformDisplay(
        EGL_PLATFORM_XCB_EXT,
        platform->xcb_connection,
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

    struct wsi_platform *platform = window->platform;

    xcb_visualid_t visualid = 0;
    EGLBoolean ok = eglGetConfigAttrib(
        dpy,
        config,
        EGL_NATIVE_VISUAL_ID,
        (EGLint *)&visualid);
    if (!ok) {
        return WSI_ERROR_EGL;
    }

    xcb_colormap_t colormap = xcb_generate_id(platform->xcb_connection);

    xcb_create_colormap_checked(
        platform->xcb_connection,
        XCB_COLORMAP_ALLOC_NONE,
        colormap,
        platform->xcb_screen->root,
        visualid);

    xcb_change_window_attributes_checked(
        platform->xcb_connection,
        window->xcb_window,
        XCB_CW_COLORMAP,
        (const uint32_t[]){ colormap });

    EGLAttrib attrs[] = {
        EGL_NONE,
    };

    *pSurface = eglCreatePlatformWindowSurface(
        dpy,
        config,
        &window->xcb_window,
        attrs);
    if (*pSurface == EGL_NO_SURFACE) {
        return WSI_ERROR_EGL;
    }

    window->api = WSI_API_EGL;
    return WSI_SUCCESS;
}
