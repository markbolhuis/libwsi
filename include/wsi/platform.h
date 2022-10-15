#ifndef WSI_INCLUDE_PLATFORM_H
#define WSI_INCLUDE_PLATFORM_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef WsiResult (*PFN_wsiCreatePlatform)(WsiPlatform *pPlatform);
typedef void (*PFN_wsiDestroyPlatform)(WsiPlatform platform);
typedef void (*PFN_wsiGetDefaultEventQueue)(WsiPlatform platform, WsiEventQueue *pEventQueue);

WsiResult
wsiCreatePlatform(WsiPlatform *pPlatform);

void
wsiDestroyPlatform(WsiPlatform platform);

void
wsiGetDefaultEventQueue(WsiPlatform platform, WsiEventQueue *pEventQueue);

#ifdef __cplusplus
}
#endif

#endif
