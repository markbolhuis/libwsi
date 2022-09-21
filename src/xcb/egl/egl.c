#include <xcb/xcb.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "wsi/egl/egl.h"

#include "../platform_priv.h"
#include "../window_priv.h"

WsiResult
wsiGetEglDisplay(
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
wsiCreateWindowEglSurface(
    WsiWindow window,
    EGLDisplay dpy,
    EGLConfig config,
    EGLSurface *pSurface)
{
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

    return WSI_SUCCESS;
}
