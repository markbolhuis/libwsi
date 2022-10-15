#ifndef WSI_SRC_WAYLAND_EVENT_QUEUE_PRIVATE_H
#define WSI_SRC_WAYLAND_EVENT_QUEUE_PRIVATE_H

struct wsi_event_queue {
    struct wl_display *wl_display;
    struct wl_event_queue *wl_event_queue; // Must be NULL if the default
};

#endif
