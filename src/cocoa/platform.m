#include <stdlib.h>
#include <string.h>

#include "utils.h"

#include "common_priv.h"
#include "platform_priv.h"
#include "window_priv.h"

static WsiResult
wsi_platform_init(const WsiPlatformCreateInfo *pCreateInfo, struct wsi_platform *platform)
{
    //TODO

    return WSI_SUCCESS;
}

static void
wsi_platform_uninit(struct wsi_platform *platform)
{
    //TODO
}

WsiResult
wsiCreatePlatform(const WsiPlatformCreateInfo *pCreateInfo, WsiPlatform *pPlatform)
{
    struct wsi_platform *p = calloc(1, sizeof(struct wsi_platform));
    if (!p) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    WsiResult result = wsi_platform_init(pCreateInfo, p);
    if (result != WSI_SUCCESS) {
        free(p);
        return result;
    }

    *pPlatform = p;
    return WSI_SUCCESS;
}

void
wsiDestroyPlatform(WsiPlatform platform)
{
    wsi_platform_uninit(platform);
    free(platform);
}

WsiResult
wsiDispatchEvents(WsiPlatform platform, int64_t timeout)
{
    @autoreleasepool {
    
    for (;;) {
        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate:[NSDate distantPast]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];
        if (event == nil)
            break;

        [NSApp sendEvent:event];
    }

    } // autoreleasepool

    return WSI_SUCCESS;
}
