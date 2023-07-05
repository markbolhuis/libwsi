#include <dlfcn.h>

#include "wsi/egl.h"

extern void *g_handle;

WsiResult
wsiGetEGLDisplay(WsiPlatform platform, EGLDisplay *pDisplay)
{
    PFN_wsiGetEGLDisplay sym
        = (PFN_wsiGetEGLDisplay)dlsym(g_handle, "wsiGetEGLDisplay");
    return sym(platform, pDisplay);
}

WsiResult
wsiCreateWindowEGLSurface(WsiWindow window, EGLDisplay dpy, EGLConfig config, EGLSurface *pSurface)
{
    PFN_wsiCreateWindowEGLSurface sym
        = (PFN_wsiCreateWindowEGLSurface)
            dlsym(g_handle, "wsiCreateWindowEGLSurface");
    return sym(window, dpy, config, pSurface);
}

void
wsiDestroyWindowEGLSurface(WsiWindow window, EGLDisplay dpy, EGLSurface surface)
{
    PFN_wsiDestroyWindowEGLSurface sym
        = (PFN_wsiDestroyWindowEGLSurface)
            dlsym(g_handle, "wsiDestroyWindowEGLSurface");
    sym(window, dpy, surface);
}
