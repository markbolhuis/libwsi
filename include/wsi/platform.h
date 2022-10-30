#ifndef WSI_INCLUDE_PLATFORM_H
#define WSI_INCLUDE_PLATFORM_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef WsiResult (*PFN_wsiCreatePlatform)(WsiPlatform *pPlatform);
typedef void (*PFN_wsiDestroyPlatform)(WsiPlatform platform);
typedef WsiEventQueue (*PFN_wsiGetDefaultEventQueue)(WsiPlatform platform);
typedef WsiResult (*PFN_wsiCreateEventQueue)(WsiPlatform platform, WsiEventQueue *pEventQueue);
typedef void (*PFN_wsiDestroyEventQueue)(WsiEventQueue eventQueue);
typedef WsiResult (*PFN_wsiDispatchEvents)(WsiEventQueue eventQueue, int64_t timeout);

WsiResult
wsiCreatePlatform(WsiPlatform *pPlatform);

void
wsiDestroyPlatform(WsiPlatform platform);

WsiEventQueue
wsiGetDefaultEventQueue(WsiPlatform platform);

WsiResult
wsiCreateEventQueue(WsiPlatform platform, WsiEventQueue *pEventQueue);

void
wsiDestroyEventQueue(WsiEventQueue eventQueue);

WsiResult
wsiDispatchEvents(WsiEventQueue eventQueue, int64_t timeout);

#ifdef __cplusplus
}
#endif

#endif
