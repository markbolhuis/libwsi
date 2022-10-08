#include <stdlib.h>

#include "wsi/window.h"

#include "platform_priv.h"
#include "window_priv.h"

void *
wsi_window_dlsym(struct wsi_window *window, const char *symbol)
{
    return wsi_platform_dlsym(window->platform, symbol);
}

WsiResult
wsiCreateWindow(
    WsiPlatform platform,
    const WsiWindowCreateInfo *pCreateInfo,
    WsiWindow *pWindow)
{
    struct wsi_window *window = calloc(1, sizeof(struct wsi_window));
    if (!window) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    PFN_wsiCreateWindow sym = wsi_platform_dlsym(platform, "wsiCreateWindow");

    enum wsi_result result = sym(
        platform->platform,
        pCreateInfo,
        &window->window);
    if (result != WSI_SUCCESS) {
        free(window);
        return result;
    }

    window->platform = platform;
    *pWindow = window;
    return WSI_SUCCESS;
}

void
wsiDestroyWindow(
    WsiWindow window)
{
    PFN_wsiDestroyWindow sym = wsi_window_dlsym(window, "wsiDestroyWindow");
    sym(window->window);
    free(window);
}

WsiResult
wsiSetWindowParent(
    WsiWindow window,
    WsiWindow parent)
{
    PFN_wsiSetWindowParent sym = wsi_window_dlsym(window, "wsiSetWindowParent");
    return sym(window->window, parent);
}

WsiResult
wsiSetWindowTitle(
    WsiWindow window,
    const char *pTitle)
{
    PFN_wsiSetWindowTitle sym = wsi_window_dlsym(window, "wsiSetWindowTitle");
    return sym(window->window, pTitle);
}
