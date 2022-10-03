#include <stdlib.h>

#include "wsi/event_queue.h"

#include "platform_priv.h"
#include "event_queue_priv.h"

void *
wsi_event_queue_dlsym(struct wsi_event_queue *event_queue, const char *symbol)
{
    return wsi_platform_dlsym(event_queue->platform, symbol);
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
