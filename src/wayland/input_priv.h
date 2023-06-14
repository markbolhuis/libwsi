#ifndef WSI_SRC_WAYLAND_INPUT_PRIVATE_H
#define WSI_SRC_WAYLAND_INPUT_PRIVATE_H

#include "wsi/input.h"

enum wsi_wl_pointer_event {
    WSI_WL_POINTER_EVENT_NONE = 0,
    WSI_WL_POINTER_EVENT_ENTER = 1,
    WSI_WL_POINTER_EVENT_LEAVE = 2,
    WSI_WL_POINTER_EVENT_MOTION = 4,
    WSI_WL_POINTER_EVENT_BUTTON = 8,
    WSI_WL_POINTER_EVENT_AXIS = 16,
    WSI_WL_POINTER_EVENT_AXIS_SOURCE = 32,
    WSI_WL_POINTER_EVENT_AXIS_STOP = 64,
    WSI_WL_POINTER_EVENT_AXIS_DISCRETE = 128,
    WSI_WL_POINTER_EVENT_AXIS_RELATIVE_DIRECTION = 256,
    WSI_WL_POINTER_EVENT_RELATIVE_MOTION = 1024,
};

struct wsi_pointer_frame {
    enum wsi_wl_pointer_event mask;

    uint32_t serial;
    uint32_t time;

    struct wl_surface *enter;
    struct wl_surface *leave;

    uint32_t button;
    uint32_t state;

    double x;
    double y;

    uint32_t axis_source;
    struct {
        double value;
        int32_t discrete;
        uint32_t start_time;
        uint32_t stop_time;
        uint32_t direction;
    } axes[2];
};

struct wsi_pointer {
    struct wsi_seat        *seat;

    struct wl_pointer      *wl_pointer;

    struct wl_cursor_theme *wl_cursor_theme;
    struct wl_cursor       *wl_cursor;
    struct wl_surface      *wl_cursor_surface;

    struct wsi_pointer_frame frame;
};

struct wsi_keyboard {
    struct wsi_seat    *seat;

    struct wl_keyboard *wl_keyboard;

    struct xkb_context *xkb_context;
    struct xkb_keymap  *xkb_keymap;
    struct xkb_state   *xkb_state;

    int32_t repeat_rate;
    int32_t repeat_delay;
};

struct wsi_seat {
    struct wsi_global  global;
    struct wl_list     link;

    struct wl_seat     *wl_seat;
    uint32_t           capabilities;
    char               *name;

    struct wsi_pointer  *pointer;
    struct wsi_keyboard *keyboard;
};

struct wsi_seat *
wsi_seat_bind(struct wsi_platform *platform, uint32_t name, uint32_t version);

void
wsi_seat_destroy(struct wsi_seat *seat);

void
wsi_seat_destroy_all(struct wsi_platform *platform);

#endif