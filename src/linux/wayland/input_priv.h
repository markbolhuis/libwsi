#ifndef WSI_SRC_WAYLAND_INPUT_PRIVATE_H
#define WSI_SRC_WAYLAND_INPUT_PRIVATE_H

#include "wsi/input.h"

enum wsi_pointer_frame_event {
    WSI_POINTER_FRAME_EVENT_NONE = 0,
    WSI_POINTER_FRAME_EVENT_ENTER = 1,
    WSI_POINTER_FRAME_EVENT_LEAVE = 2,
    WSI_POINTER_FRAME_EVENT_MOTION = 4,
    WSI_POINTER_FRAME_EVENT_BUTTON = 8,
    WSI_POINTER_FRAME_EVENT_AXIS_SOURCE = 16,
    WSI_POINTER_FRAME_EVENT_RELATIVE_MOTION = 32,
    WSI_POINTER_FRAME_EVENT_STATE = 64,
};

enum wsi_pointer_frame_axis_event {
    WSI_POINTER_FRAME_AXIS_EVENT_NONE = 0,
    WSI_POINTER_FRAME_AXIS_EVENT_START = 1,
    WSI_POINTER_FRAME_AXIS_EVENT_STOP = 2,
    WSI_POINTER_FRAME_AXIS_EVENT_DISCRETE = 4,
    WSI_POINTER_FRAME_AXIS_EVENT_DIRECTION = 8,
};

enum wsi_pointer_state {
    WSI_POINTER_STATE_NONE = 0,
    WSI_POINTER_STATE_LOCKED = 1,
    WSI_POINTER_STATE_CONFINED = 2,
};

struct wsi_pointer_frame {
    enum wsi_pointer_frame_event mask;

    uint32_t serial;
    int64_t time;

    enum wsi_pointer_state state;

    struct wl_surface *enter;
    struct wl_surface *leave;

    struct {
        uint32_t code;
        uint32_t state;
    } button;

    double x;
    double y;
    double dx;
    double dy;
    double udx;
    double udy;

    struct {
        struct {
            enum wsi_pointer_frame_axis_event mask;
            double value;
            double discrete;
            uint32_t direction;
        } h, v;
        uint32_t source;
    } axis;
};

struct wsi_pointer_constraint {
    struct wl_list link;
    struct wsi_pointer *pointer;
    struct wl_surface *wl_surface;
    union {
        struct zwp_locked_pointer_v1 *wp_locked_pointer_v1;
        struct zwp_confined_pointer_v1 *wp_confined_pointer_v1;
    };
    uint32_t type;
    uint32_t lifetime;
};

struct wsi_pointer {
    struct wl_pointer              *wl_pointer;
    struct zwp_input_timestamps_v1 *wp_timestamps_v1;
    struct zwp_relative_pointer_v1 *wp_relative_v1;

    struct wl_cursor_theme *wl_cursor_theme;
    struct wl_cursor       *wl_cursor;
    struct wl_surface      *wl_surface;

    struct wsi_pointer_frame frame;

    struct wl_list constraints;
};

struct wsi_keyboard {
    struct wl_keyboard             *wl_keyboard;
    struct zwp_input_timestamps_v1 *wp_timestamps_v1;

    struct xkb_context       *xkb_context;
    struct xkb_keymap        *xkb_keymap;
    struct xkb_state         *xkb_state;
    struct xkb_compose_state *xkb_compose_state;

    int32_t repeat_rate;
    int32_t repeat_delay;
    int64_t event_time;
};

struct wsi_shortcuts_inhibitor {
    struct wl_list link;
    struct wsi_seat *seat;
    struct wl_surface *wl_surface;
    struct zwp_keyboard_shortcuts_inhibitor_v1 *wp_shortcuts_inhibitor_v1;
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

    struct wl_list shortcut_inhibitors;
};

void
wsi_pointer_constrain(
    struct wsi_pointer *pointer,
    uint32_t type,
    struct wl_surface *wl_surface,
    bool persistent,
    double pos_hint[2]);

void
wsi_pointer_remove_constraint(struct wsi_pointer *pointer, struct wl_surface *wl_surface);

void
wsi_seat_inhibit_shortcuts(struct wsi_seat *seat, struct wl_surface *wl_surface);

void
wsi_seat_restore_shortcuts(struct wsi_seat *seat, struct wl_surface *wl_surface);

void
wsi_seat_set_idle_timer(struct wsi_seat *seat, const uint32_t *time);

struct wsi_seat *
wsi_seat_add(struct wsi_platform *platform, uint32_t name, uint32_t version);

void
wsi_seat_remove(struct wsi_seat *seat);

void
wsi_seat_destroy_all(struct wsi_platform *platform);

#endif