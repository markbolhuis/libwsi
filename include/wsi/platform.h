#ifndef WSI_INCLUDE_PLATFORM_H
#define WSI_INCLUDE_PLATFORM_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WsiPlatformCreateInfo {
    WsiStructureType sType;
    const void *pNext;
} WsiPlatformCreateInfo;

typedef WsiResult (*PFN_wsiCreatePlatform)(const WsiPlatformCreateInfo *pCreateInfo, WsiPlatform *pPlatform);
typedef void (*PFN_wsiDestroyPlatform)(WsiPlatform platform);
typedef WsiResult (*PFN_wsiDispatchEvents)(WsiPlatform platform, int64_t timeout);

WsiResult
wsiCreatePlatform(const WsiPlatformCreateInfo *pCreateInfo, WsiPlatform *pPlatform);

void
wsiDestroyPlatform(WsiPlatform platform);

WsiResult
wsiDispatchEvents(WsiPlatform platform, int64_t timeout);

#ifdef __cplusplus
}
#endif

#endif
