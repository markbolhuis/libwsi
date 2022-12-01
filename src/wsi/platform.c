#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

#include "wsi/platform.h"

void* g_handle = NULL;

static void __attribute__((constructor))
init(void)
{
    const char *session = getenv("XDG_SESSION_TYPE");
    if (session == NULL) {
        fprintf(stderr, "XDG_SESSION_TYPE not set\n");
        exit(EXIT_FAILURE);
    }

    void *handle = NULL;
    if (strcmp(session, "wayland") == 0) {
        handle = dlopen("libwsi-wl.so", RTLD_NOW);
    } else if (strcmp(session, "x11") == 0) {
        handle = dlopen("libwsi-x11.so", RTLD_NOW);
    } else {
        fprintf(stderr, "Unsupported XDG_SESSION_TYPE: %s\n", session);
        exit(EXIT_FAILURE);
    }

    if (handle == NULL) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    g_handle = handle;
}

static void __attribute__((destructor))
fini(void)
{
    if (g_handle) {
        dlclose(g_handle);
    }
}

WsiResult
wsiCreatePlatform(const WsiPlatformCreateInfo *pCreateInfo, WsiPlatform *pPlatform)
{
    PFN_wsiCreatePlatform sym
        = (PFN_wsiCreatePlatform)dlsym(g_handle, "wsiCreatePlatform");
    return sym(pCreateInfo, pPlatform);
}

void
wsiDestroyPlatform(WsiPlatform platform)
{
    PFN_wsiDestroyPlatform sym
        = (PFN_wsiDestroyPlatform)dlsym(g_handle, "wsiDestroyPlatform");
    sym(platform);
}

WsiEventQueue
wsiGetDefaultEventQueue(WsiPlatform platform)
{
    PFN_wsiGetDefaultEventQueue sym
        = (PFN_wsiGetDefaultEventQueue)dlsym(g_handle, "wsiGetDefaultEventQueue");
    return sym(platform);
}

WsiResult
wsiCreateEventQueue(WsiPlatform platform, const WsiEventQueueCreateInfo *pCreateInfo, WsiEventQueue *pEventQueue)
{
    PFN_wsiCreateEventQueue sym
        = (PFN_wsiCreateEventQueue)dlsym(g_handle, "wsiCreateEventQueue");
    return sym(platform, pCreateInfo, pEventQueue);
}

void
wsiDestroyEventQueue(WsiEventQueue eventQueue)
{
    PFN_wsiDestroyEventQueue sym
        = (PFN_wsiDestroyEventQueue)dlsym(g_handle, "wsiDestroyEventQueue");
    sym(eventQueue);
}

WsiResult
wsiDispatchEvents(WsiEventQueue eventQueue, int64_t timeout)
{
    PFN_wsiDispatchEvents sym
        = (PFN_wsiDispatchEvents)dlsym(g_handle, "wsiDispatchEvents");
    return sym(eventQueue, timeout);
}
