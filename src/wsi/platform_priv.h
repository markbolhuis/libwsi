#ifndef WSI_SRC_WSI_PLATFORM_PRIVATE_H
#define WSI_SRC_WSI_PLATFORM_PRIVATE_H

struct wsi_platform {
    void *handle;
    struct wsi_platform *platform;
};

struct wsi_event_queue {
    struct wsi_platform *platform;
    struct wsi_event_queue *event_queue;
};

void *
wsi_platform_dlsym(struct wsi_platform *platform, const char *symbol);

void *
wsi_event_queue_dlsym(struct wsi_event_queue *event_queue, const char *symbol);

#endif
