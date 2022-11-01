#include <dlfcn.h>

#include "wsi/window.h"

extern void *g_handle;

WsiResult
wsiCreateWindow(
    WsiPlatform platform,
    const WsiWindowCreateInfo *pCreateInfo,
    WsiWindow *pWindow)
{
    PFN_wsiCreateWindow sym
        = (PFN_wsiCreateWindow)dlsym(g_handle, "wsiCreateWindow");
    return sym(platform, pCreateInfo, pWindow);
}

void
wsiDestroyWindow(WsiWindow window)
{
    PFN_wsiDestroyWindow sym
        = (PFN_wsiDestroyWindow)dlsym(g_handle, "wsiDestroyWindow");
    sym(window);
}

WsiResult
wsiSetWindowParent(WsiWindow window, WsiWindow parent)
{
    PFN_wsiSetWindowParent sym
        = (PFN_wsiSetWindowParent)dlsym(g_handle, "wsiSetWindowParent");
    return sym(window, parent);
}

WsiResult
wsiSetWindowTitle(WsiWindow window, const char *pTitle)
{
    PFN_wsiSetWindowTitle sym
        = (PFN_wsiSetWindowTitle)dlsym(g_handle, "wsiSetWindowTitle");
    return sym(window, pTitle);
}
