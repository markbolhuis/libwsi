#ifndef WSI_SRC_WAYLAND_SEAT_PRIVATE_H
#define WSI_SRC_WAYLAND_SEAT_PRIVATE_H

struct wsi_seat_ref {
    struct wl_list link;
    uint64_t id;
    uint32_t name;
    uint32_t version;
    struct wsi_seat *seat;
};

struct wsi_seat {
    struct wsi_global  global;

    struct wl_seat     *wl_seat;
    uint32_t           capabilities;
    char               *name;

    struct wsi_pointer  *pointer;
    struct wsi_keyboard *keyboard;
};

bool
wsi_seat_ref_add(struct wsi_platform *platform, uint32_t name, uint32_t version);

void
wsi_seat_ref_remove(struct wsi_seat_ref *seat);

void
wsi_seat_ref_remove_all(struct wsi_platform *platform);

#endif