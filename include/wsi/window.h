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
    bool decoration;
} WsiWindowFeatures;

typedef struct wsi_window_create_info {
    WsiEventQueue eventQueue;
    WsiWindow parent;
    uint32_t width;
    uint32_t height;
    const char *pTitle;
} WsiWindowCreateInfo;

WsiResult
wsiCreateWindow(WsiPlatform platform, const WsiWindowCreateInfo *pCreateInfo, WsiWindow *pWindow);

void
wsiDestroyWindow(WsiPlatform platform, WsiWindow window);

void
wsiGetWindowFeatures(WsiWindow window, WsiWindowFeatures *pFeatures);

WsiResult
wsiGetWindowParent(WsiWindow window, WsiWindow *pParent);

WsiResult
wsiSetWindowParent(WsiWindow window, WsiWindow parent);

void
wsiGetWindowExtent(WsiWindow window, uint32_t *width, uint32_t *height);

WsiResult
wsiSetWindowTitle(WsiWindow window, const char *pTitle);

#ifdef __cplusplus
}
#endif

#endif
