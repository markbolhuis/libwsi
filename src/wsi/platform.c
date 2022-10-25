#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

#include "wsi/platform.h"

#include "platform_priv.h"

void *
wsi_platform_dlsym(struct wsi_platform *platform, const char *symbol)
{
    return dlsym(platform->handle, symbol);
}

void *
wsi_event_queue_dlsym(struct wsi_event_queue *event_queue, const char *symbol)
{
    return wsi_platform_dlsym(event_queue->platform, symbol);
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
        platform->handle = dlopen("libwsi-x11.so", RTLD_NOW);
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

WsiEventQueue
wsiGetDefaultEventQueue(WsiPlatform platform)
{
    struct wsi_event_queue *eq = calloc(1, sizeof(struct wsi_event_queue));
    assert(eq != NULL);

    eq->platform = platform;

    PFN_wsiGetDefaultEventQueue sym = wsi_platform_dlsym(platform, "wsiGetDefaultEventQueue");
    eq->event_queue = sym(platform->platform);

    return eq;
}

WsiResult
wsiCreateEventQueue(WsiPlatform platform, WsiEventQueue *pEventQueue)
{
    struct wsi_event_queue *event_queue = calloc(1, sizeof(struct wsi_event_queue));
    if (!event_queue) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    PFN_wsiCreateEventQueue sym = wsi_platform_dlsym(platform, "wsiCreateEventQueue");
    enum wsi_result result = sym(platform->platform, &event_queue->event_queue);

    if (result != WSI_SUCCESS) {
        free(event_queue);
        return result;
    }

    event_queue->platform = platform;
    *pEventQueue = event_queue;
    return WSI_SUCCESS;
}

void
wsiDestroyEventQueue(WsiEventQueue eventQueue)
{
    PFN_wsiDestroyEventQueue sym = wsi_event_queue_dlsym(eventQueue, "wsiDestroyEventQueue");
    sym(eventQueue->event_queue);
    free(eventQueue);
}

WsiResult
wsiPollEventQueue(WsiEventQueue eventQueue)
{
    PFN_wsiPollEventQueue sym = wsi_event_queue_dlsym(eventQueue, "wsiPollEventQueue");
    return sym(eventQueue->event_queue);
}
