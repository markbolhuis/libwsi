#ifndef WSI_INCLUDE_WINDOW_H
#define WSI_INCLUDE_WINDOW_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WsiConfigureWindowEvent {
    WsiEvent base;
    WsiWindow window;
    WsiExtent extent;
} WsiConfigureWindowEvent;

typedef struct WsiCloseWindowEvent {
    WsiEvent base;
    WsiWindow window;
} WsiCloseWindowEvent;

typedef void (*PFN_wsiConfigureWindow)(void *pUserData, const WsiConfigureWindowEvent *pConfig);
typedef void (*PFN_wsiCloseWindow)(void *pUserData, const WsiCloseWindowEvent *pClose);

typedef struct WsiWindowCreateInfo {
    WsiStructureType sType;
    const void *pNext;
    WsiWindow parent;
    WsiExtent extent;
    const char *pTitle;
    void *pUserData;
    PFN_wsiConfigureWindow pfnConfigureWindow;
    PFN_wsiCloseWindow pfnCloseWindow;
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
