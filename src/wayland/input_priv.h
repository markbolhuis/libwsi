#ifndef WSI_SRC_WAYLAND_INPUT_PRIVATE_H
#define WSI_SRC_WAYLAND_INPUT_PRIVATE_H

#include "wsi/input.h"

enum wsi_wl_pointer_event {
    WSI_WL_POINTER_EVENT_NONE = 0,
    WSI_WL_POINTER_EVENT_ENTER = 1,
    WSI_WL_POINTER_EVENT_LEAVE = 2,
    WSI_WL_POINTER_EVENT_MOTION = 4,
    WSI_WL_POINTER_EVENT_BUTTON = 8,
    WSI_WL_POINTER_EVENT_AXIS_SOURCE = 16,
    WSI_WL_POINTER_EVENT_RELATIVE_MOTION = 32,
    WSI_WL_POINTER_EVENT_LOCKED = 64,
    WSI_WL_POINTER_EVENT_UNLOCKED = 128,
    WSI_WL_POINTER_EVENT_CONFINED = 256,
    WSI_WL_POINTER_EVENT_UNCONFINED = 512,
};

enum wsi_wl_axis_event {
    WSI_WL_AXIS_EVENT_NONE = 0,
    WSI_WL_AXIS_EVENT_START = 1,
    WSI_WL_AXIS_EVENT_STOP = 2,
    WSI_WL_AXIS_EVENT_DISCRETE = 4,
    WSI_WL_AXIS_EVENT_DIRECTION = 8,
};

struct wsi_pointer_frame {
    enum wsi_wl_pointer_event mask;

    uint32_t serial;
    int64_t time;

    struct wl_surface *enter;
    struct wl_surface *leave;

    uint32_t button;
    uint32_t state;

    double x;
    double y;
    double dx;
    double dy;
    double udx;
    double udy;

    uint32_t axis_source;
    struct {
        enum wsi_wl_axis_event mask;
        double value;
        int32_t discrete;
        uint32_t direction;
    } axes[2];
};

struct wsi_pointer {
    struct wl_pointer              *wl_pointer;
    struct zwp_input_timestamps_v1 *wp_timestamps_v1;
    struct zwp_relative_pointer_v1 *wp_relative_v1;
    struct zwp_confined_pointer_v1 *wp_confined_v1;
    struct zwp_locked_pointer_v1   *wp_locked_v1;

    struct wl_cursor_theme *wl_cursor_theme;
    struct wl_cursor       *wl_cursor;
    struct wl_surface      *wl_cursor_surface;

    struct wsi_pointer_frame frame;
    uint32_t constraint_lifetime;
};

struct wsi_keyboard {
    struct wl_keyboard             *wl_keyboard;
    struct zwp_input_timestamps_v1 *wp_timestamps_v1;
    struct zwp_keyboard_shortcuts_inhibitor_v1 *wp_shortcuts_inhibitor_v1;

    struct xkb_context *xkb_context;
    struct xkb_keymap  *xkb_keymap;
    struct xkb_state   *xkb_state;

    int32_t repeat_rate;
    int32_t repeat_delay;
    int64_t event_time;
};

struct wsi_seat {
    struct wsi_global global;
    struct wl_list    link;

    struct wl_seat                  *wl_seat;
    struct ext_idle_notification_v1 *ext_idle_notification_v1;

    uint32_t capabilities;
    char     *name;

    struct wsi_pointer  pointer;
    struct wsi_keyboard keyboard;
};

struct wsi_seat *
wsi_seat_add(struct wsi_platform *platform, uint32_t name, uint32_t version);

void
wsi_seat_remove(struct wsi_seat *seat);

void
wsi_seat_destroy_all(struct wsi_platform *platform);

#endif