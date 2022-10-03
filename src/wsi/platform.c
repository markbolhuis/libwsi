#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "wsi/platform.h"

#include "platform_priv.h"

void *
wsi_platform_dlsym(struct wsi_platform *platform, const char *symbol)
{
    return dlsym(platform->handle, symbol);
}

WsiResult
wsiCreatePlatform(WsiPlatform *pPlatform)
{
    struct wsi_platform *platform = calloc(1, sizeof(struct wsi_platform));
    if (!platform) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    const char *session = getenv("XDG_SESSION_TYPE");
    if (!session) {
        free(platform);
        return WSI_ERROR_PLATFORM;
    }

    if (strcmp(session, "wayland") == 0) {
        platform->handle = dlopen("libwsi-wl.so", RTLD_NOW);
    } else if (strcmp(session, "x11") == 0) {
        platform->handle = dlopen("libwsi-xcb.so", RTLD_NOW);
    } else {
        free(platform);
        return WSI_ERROR_PLATFORM;
    }

    if (!platform->handle) {
        fprintf(stderr, "%s: %s\n", __func__, dlerror());
        free(platform);
        return WSI_ERROR_PLATFORM;
    }

    PFN_wsiCreatePlatform sym = wsi_platform_dlsym(platform, "wsiCreatePlatform");
    enum wsi_result result = sym(&platform->platform);
    if (result != WSI_SUCCESS) {
        dlclose(platform->handle);
        free(platform);
        return result;
    }

    *pPlatform = platform;
    return WSI_SUCCESS;
}

void
wsiDestroyPlatform(WsiPlatform platform)
{
    PFN_wsiDestroyPlatform sym = wsi_platform_dlsym(platform, "wsiDestroyPlatform");
    sym(platform->platform);
    dlclose(platform->handle);
    free(platform);
}

void
wsiGetPlatformFeatures(WsiPlatform platform, WsiPlatformFeatures *pFeatures)
{
    PFN_wsiGetPlatformFeatures sym = wsi_platform_dlsym(platform, "wsiGetPlatformFeatures");
    sym(platform->platform, pFeatures);
}

void
wsiGetPlatformLimits(WsiPlatform platform, WsiPlatformLimits *pLimits)
{
    PFN_wsiGetPlatformLimits sym = wsi_platform_dlsym(platform, "wsiGetPlatformLimits");
    sym(platform->platform, pLimits);
}

void
wsiPoll(WsiPlatform platform)
{
    PFN_wsiPoll sym = wsi_platform_dlsym(platform, "wsiPoll");
    sym(platform->platform);
}
