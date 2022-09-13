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
