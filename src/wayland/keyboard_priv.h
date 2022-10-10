#ifndef WSI_SRC_WAYLAND_KEYBOARD_PRIVATE_H
#define WSI_SRC_WAYLAND_KEYBOARD_PRIVATE_H

struct wsi_keyboard {
    struct wsi_seat    *seat;

    struct wl_keyboard *wl_keyboard;

    struct xkb_context *xkb_context;
    struct xkb_keymap  *xkb_keymap;
    struct xkb_state   *xkb_state;

    int32_t repeat_rate;
    int32_t repeat_delay;
};

bool
wsi_keyboard_create(struct wsi_seat *seat);

void
wsi_keyboard_destroy(struct wsi_seat *seat);

#endif
