#include "wsi/egl/egl.h"

#include "../platform_priv.h"
#include "../window_priv.h"

typedef WsiResult (*PFN_wsiGetEGLDisplay)(WsiPlatform platform, EGLDisplay *pDisplay);
typedef WsiResult (*PFN_wsiCreateWindowEGLSurface)(WsiWindow window, EGLDisplay dpy, EGLConfig config, EGLSurface *pSurface);

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
