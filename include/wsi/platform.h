#ifndef WSI_INCLUDE_PLATFORM_H
#define WSI_INCLUDE_PLATFORM_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WsiEventQueueCreateInfo {
    WsiStructureType sType;
    const void *pNext;
} WsiEventQueueCreateInfo;

typedef struct WsiPlatformCreateInfo {
    WsiStructureType sType;
    const void *pNext;
    const WsiEventQueueCreateInfo *pEventQueueInfo;
} WsiPlatformCreateInfo;

typedef WsiResult (*PFN_wsiCreatePlatform)(const WsiPlatformCreateInfo *pCreateInfo, WsiPlatform *pPlatform);
typedef void (*PFN_wsiDestroyPlatform)(WsiPlatform platform);
typedef WsiEventQueue (*PFN_wsiGetDefaultEventQueue)(WsiPlatform platform);
typedef WsiResult (*PFN_wsiCreateEventQueue)(WsiPlatform platform, const WsiEventQueueCreateInfo *pCreateInfo, WsiEventQueue *pEventQueue);
typedef void (*PFN_wsiDestroyEventQueue)(WsiEventQueue eventQueue);
typedef WsiResult (*PFN_wsiDispatchEvents)(WsiEventQueue eventQueue, int64_t timeout);

WsiResult
wsiCreatePlatform(const WsiPlatformCreateInfo *pCreateInfo, WsiPlatform *pPlatform);

void
wsiDestroyPlatform(WsiPlatform platform);

WsiEventQueue
wsiGetDefaultEventQueue(WsiPlatform platform);

WsiResult
wsiCreateEventQueue(WsiPlatform platform, const WsiEventQueueCreateInfo *pCreateInfo, WsiEventQueue *pEventQueue);

void
wsiDestroyEventQueue(WsiEventQueue eventQueue);

WsiResult
wsiDispatchEvents(WsiEventQueue eventQueue, int64_t timeout);

#ifdef __cplusplus
}
#endif

#endif
