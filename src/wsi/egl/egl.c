#include "wsi/egl/egl.h"

#include "../platform_priv.h"
#include "../window_priv.h"

typedef WsiResult (*PFN_wsiGetEglDisplay)(WsiPlatform platform, EGLDisplay *pDisplay);
typedef WsiResult (*PFN_wsiCreateWindowEglSurface)(WsiWindow window, EGLDisplay dpy, EGLConfig config, EGLSurface *pSurface);

WsiResult
wsiGetEglDisplay(WsiPlatform platform, EGLDisplay *pDisplay)
{
    PFN_wsiGetEglDisplay sym = wsi_platform_dlsym(platform, "wsiGetEglDisplay");
    return sym(platform->platform, pDisplay);
}

WsiResult
wsiCreateWindowEglSurface(WsiWindow window, EGLDisplay dpy, EGLConfig config, EGLSurface *pSurface)
{
    PFN_wsiCreateWindowEglSurface sym = wsi_window_dlsym(window, "wsiCreateWindowEglSurface");
    return sym(window->window, dpy, config, pSurface);
}
