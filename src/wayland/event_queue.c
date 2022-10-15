#include <stdlib.h>
#include <assert.h>
#include <poll.h>

#include <wayland-client-core.h>

#include "wsi/event_queue.h"

#include "event_queue_priv.h"
#include "platform_priv.h"

// wayland doesn't give direct access to the default event queue, so we have to
// use wl_display, and it's associated functions.

static inline int
wsi_event_queue_prepare_read(struct wsi_event_queue *eq)
{
    struct wl_display *display = eq->wl_display;
    if (eq->wl_event_queue) {
        return wl_display_prepare_read_queue(display, eq->wl_event_queue);
    }
    return wl_display_prepare_read(display);
}

static inline int
wsi_event_queue_dispatch_pending(struct wsi_event_queue *eq)
{
    struct wl_display *display = eq->wl_display;
    if (eq->wl_event_queue) {
        return wl_display_dispatch_queue_pending(display, eq->wl_event_queue);
    }
    return wl_display_dispatch_pending(display);
}

WsiResult
wsiCreateEventQueue(WsiPlatform platform, WsiEventQueue *pEventQueue)
{
    struct wsi_event_queue *eq = calloc(1, sizeof(struct wsi_event_queue));
    if (!eq) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    eq->wl_display = platform->wl_display;
    eq->wl_event_queue = wl_display_create_queue(platform->wl_display);
    if (!eq->wl_event_queue) {
        free(eq);
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    *pEventQueue = eq;
    return WSI_SUCCESS;
}

void
wsiDestroyEventQueue(WsiEventQueue eventQueue)
{
    // Only non-default event queues are created by wsiCreateEventQueue
    // and are destroyed here.
    assert(eventQueue->wl_event_queue);
    wl_event_queue_destroy(eventQueue->wl_event_queue);
    free(eventQueue);
}

WsiResult
wsiPollEventQueue(WsiEventQueue eventQueue)
{
    struct wl_display *wl_display = eventQueue->wl_display;

    if (wsi_event_queue_prepare_read(eventQueue) == -1) {
        wsi_event_queue_dispatch_pending(eventQueue);
        return WSI_SUCCESS;
    }

    wl_display_flush(wl_display);

    struct pollfd pfd = {
        .fd = wl_display_get_fd(wl_display),
        .events = POLLIN,
        .revents = 0,
    };

    int n = poll(&pfd, 1, 0);
    if (n == -1) {
        wl_display_cancel_read(wl_display);
        return WSI_ERROR_PLATFORM;
    }

    if (pfd.revents & POLLIN) {
        wl_display_read_events(wl_display);
    } else {
        wl_display_cancel_read(wl_display);
    }

    wsi_event_queue_dispatch_pending(eventQueue);
    return WSI_SUCCESS;
}
