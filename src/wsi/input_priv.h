#ifndef WSI_SRC_WSI_INPUT_PRIVATE_H
#define WSI_SRC_WSI_INPUT_PRIVATE_H

struct wsi_seat {
    struct wsi_platform *platform;
    struct wsi_seat *seat;
};

void *
wsi_seat_dlsym(struct wsi_seat *seat, const char *symbol);

#endif
