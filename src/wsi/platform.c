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

static enum wsi_result
wsi_platform_dlopen(struct wsi_platform *platform)
{
    const char *session = getenv("XDG_SESSION_TYPE");
    if (!session) {
        return WSI_ERROR_PLATFORM;
    }

    if (strcmp(session, "wayland") == 0) {
        platform->handle = dlopen("libwsi-wl.so", RTLD_NOW);
    } else if (strcmp(session, "x11") == 0) {
        platform->handle = dlopen("libwsi-xcb.so", RTLD_NOW);
    } else {
        return WSI_ERROR_PLATFORM;
    }

    if (!platform->handle) {
        fprintf(stderr, "%s: %s\n", __func__, dlerror());
        return WSI_ERROR_PLATFORM;
    }

    return WSI_SUCCESS;
}

WsiResult
wsiCreatePlatform(WsiPlatform *pPlatform)
{
    struct wsi_platform *platform = calloc(1, sizeof(struct wsi_platform));
    if (!platform) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    enum wsi_result result = wsi_platform_dlopen(platform);
    if (result != WSI_SUCCESS) {
        goto err_dlopen;
    }

    PFN_wsiCreatePlatform sym = wsi_platform_dlsym(platform, "wsiCreatePlatform");
    result = sym(&platform->platform);
    if (result != WSI_SUCCESS) {
        goto err_platform;
    }

    *pPlatform = platform;
    return result;

err_platform:
    dlclose(platform->handle);
err_dlopen:
    free(platform);
    return result;
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
