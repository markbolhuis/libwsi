#ifndef WSI_INCLUDE_PLATFORM_H
#define WSI_INCLUDE_PLATFORM_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wsi_platform_features {
    bool windowing;
    bool overlay;
    bool background;
    bool fullscreen;
} WsiPlatformFeatures;

typedef struct wsi_platform_limits {
    uint32_t maxEventQueueCount;
    uint32_t maxSeatCount;
    uint32_t maxWindowWidth;
    uint32_t maxWindowHeight;
} WsiPlatformLimits;

typedef WsiResult (*PFN_wsiCreatePlatform)(WsiPlatform *pPlatform);
typedef void (*PFN_wsiDestroyPlatform)(WsiPlatform platform);
typedef void (*PFN_wsiGetPlatformFeatures)(WsiPlatform platform, WsiPlatformFeatures *pFeatures);
typedef void (*PFN_wsiGetPlatformLimits)(WsiPlatform platform, WsiPlatformLimits *pLimits);
typedef void (*PFN_wsiPoll)(WsiPlatform platform);

WsiResult
wsiCreatePlatform(WsiPlatform *pPlatform);

void
wsiDestroyPlatform(WsiPlatform platform);

void
wsiGetPlatformFeatures(WsiPlatform platform, WsiPlatformFeatures *pFeatures);

void
wsiGetPlatformLimits(WsiPlatform platform, WsiPlatformLimits *pLimits);

void
wsiPoll(WsiPlatform platform);

#ifdef __cplusplus
}
#endif

#endif
