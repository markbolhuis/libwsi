#ifndef WSI_INCLUDE_WINDOW_H
#define WSI_INCLUDE_WINDOW_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    WsiEvent base;
    WsiWindow window;
} WsiCloseWindowEvent;

typedef struct {
    WsiEvent base;
    WsiWindow window;
    WsiExtent extent;
} WsiResizeWindowEvent;

typedef struct {
    WsiEventQueue eventQueue;
    WsiWindow parent;
    WsiExtent extent;
    const char *pTitle;
} WsiWindowCreateInfo;

typedef WsiResult (*PFN_wsiCreateWindow)(WsiPlatform platform, const WsiWindowCreateInfo *pCreateInfo, WsiWindow *pWindow);
typedef void (*PFN_wsiDestroyWindow)(WsiWindow window);
typedef WsiResult (*PFN_wsiSetWindowParent)(WsiWindow window, WsiWindow parent);
typedef WsiResult (*PFN_wsiSetWindowTitle)(WsiWindow window, const char *pTitle);

WsiResult
wsiCreateWindow(WsiPlatform platform, const WsiWindowCreateInfo *pCreateInfo, WsiWindow *pWindow);

void
wsiDestroyWindow(WsiWindow window);

WsiResult
wsiSetWindowParent(WsiWindow window, WsiWindow parent);

WsiResult
wsiSetWindowTitle(WsiWindow window, const char *pTitle);

#ifdef __cplusplus
}
#endif

#endif
