#ifndef WSI_INCLUDE_WINDOW_H
#define WSI_INCLUDE_WINDOW_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wsi_window_features {
    bool fullscreen;
    bool minimize;
    bool maximize;
    bool resize;
    bool move;
} WsiWindowFeatures;

typedef struct wsi_window_create_info {
    WsiEventQueue eventQueue;
    WsiWindow parent;
    WsiExtent extent;
    const char *pTitle;
} WsiWindowCreateInfo;

WsiResult
wsiCreateWindow(WsiPlatform platform, const WsiWindowCreateInfo *pCreateInfo, WsiWindow *pWindow);

void
wsiDestroyWindow(WsiPlatform platform, WsiWindow window);

void
wsiGetWindowFeatures(WsiWindow window, WsiWindowFeatures *pFeatures);

WsiResult
wsiSetWindowParent(WsiWindow window, WsiWindow parent);

void
wsiGetWindowExtent(WsiWindow window, WsiExtent *pExtent);

WsiResult
wsiSetWindowTitle(WsiWindow window, const char *pTitle);

bool
wsiShouldCloseWindow(WsiWindow window);

#ifdef __cplusplus
}
#endif

#endif
