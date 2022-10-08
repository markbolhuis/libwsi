#ifndef WSI_INCLUDE_PLATFORM_H
#define WSI_INCLUDE_PLATFORM_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef WsiResult (*PFN_wsiCreatePlatform)(WsiPlatform *pPlatform);
typedef void (*PFN_wsiDestroyPlatform)(WsiPlatform platform);
typedef void (*PFN_wsiPoll)(WsiPlatform platform);

WsiResult
wsiCreatePlatform(WsiPlatform *pPlatform);

void
wsiDestroyPlatform(WsiPlatform platform);

void
wsiPoll(WsiPlatform platform);

#ifdef __cplusplus
}
#endif

#endif
