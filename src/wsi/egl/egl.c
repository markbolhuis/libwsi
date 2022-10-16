#include "wsi/egl/egl.h"

#include "../platform_priv.h"
#include "../window_priv.h"

WsiResult
wsiGetEGLDisplay(WsiPlatform platform, EGLDisplay *pDisplay)
{
    PFN_wsiGetEGLDisplay sym = wsi_platform_dlsym(platform, "wsiGetEGLDisplay");
    return sym(platform->platform, pDisplay);
}

WsiResult
wsiCreateWindowEGLSurface(WsiWindow window, EGLDisplay dpy, EGLConfig config, EGLSurface *pSurface)
{
    PFN_wsiCreateWindowEGLSurface sym = wsi_window_dlsym(window, "wsiCreateWindowEGLSurface");
    return sym(window->window, dpy, config, pSurface);
}

void
wsiDestroyWindowEGLSurface(WsiWindow window, EGLDisplay dpy, EGLSurface surface)
{
    PFN_wsiDestroyWindowEGLSurface sym = wsi_window_dlsym(window, "wsiDestroyWindowEGLSurface");
    sym(window->window, dpy, surface);
}
