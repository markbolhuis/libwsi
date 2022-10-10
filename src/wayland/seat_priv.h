#ifndef WSI_SRC_WAYLAND_SEAT_PRIVATE_H
#define WSI_SRC_WAYLAND_SEAT_PRIVATE_H

struct wsi_seat {
    struct wsi_global  global;

    struct wl_list     link;
    uint64_t           id;
    int                ref_count;

    struct wl_seat     *wl_seat;
    uint32_t           capabilities;
    char               *name;

    struct wsi_keyboard *keyboard;
};

struct wsi_seat *
wsi_seat_bind(struct wsi_platform *platform, uint32_t name, uint32_t version);

void
wsi_seat_destroy(struct wsi_seat *seat);

void
wsi_seat_destroy_all(struct wsi_platform *platform);

#endif