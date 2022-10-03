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

typedef WsiResult (*PFN_wsiCreateWindow)(WsiPlatform platform, const WsiWindowCreateInfo *pCreateInfo, WsiWindow *pWindow);
typedef void (*PFN_wsiDestroyWindow)(WsiWindow window);
typedef void (*PFN_wsiGetWindowExtent)(WsiWindow window, WsiExtent *pExtent);
typedef void (*PFN_wsiGetWindowFeatures)(WsiWindow window, WsiWindowFeatures *pFeatures);
typedef WsiResult (*PFN_wsiSetWindowParent)(WsiWindow window, WsiWindow parent);
typedef WsiResult (*PFN_wsiSetWindowTitle)(WsiWindow window, const char *pTitle);
typedef bool (*PFN_wsiShouldCloseWindow)(WsiWindow window);

WsiResult
wsiCreateWindow(WsiPlatform platform, const WsiWindowCreateInfo *pCreateInfo, WsiWindow *pWindow);

void
wsiDestroyWindow(WsiWindow window);

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
