#ifndef WSI_SRC_WAYLAND_EVENT_QUEUE_PRIVATE_H
#define WSI_SRC_WAYLAND_EVENT_QUEUE_PRIVATE_H

struct wsi_event_queue {
    struct wsi_platform *platform;
    struct wl_event_queue *wl_event_queue;
};

#endif
