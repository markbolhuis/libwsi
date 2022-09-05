#include <stdlib.h>

#include <wayland-client.h>

#include "wsi/event_queue.h"

#include "platform_priv.h"
#include "event_queue_priv.h"

WsiResult
wsiCreateEventQueue(
    WsiPlatform platform,
    WsiEventQueue *pEventQueue)
{
    struct wsi_event_queue *event_queue = calloc(1, sizeof(struct wsi_event_queue));
    if (!event_queue) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    event_queue->wl_event_queue = wl_display_create_queue(platform->wl_display);
    if (!event_queue->wl_event_queue) {
        free(event_queue);
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    *pEventQueue = event_queue;
    return WSI_SUCCESS;
}

void
wsiDestroyEventQueue(
    WsiPlatform platform,
    WsiEventQueue eventQueue)
{
    wl_event_queue_destroy(eventQueue->wl_event_queue);
    free(eventQueue);
}
