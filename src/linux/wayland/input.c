#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <input-timestamps-unstable-v1-client-protocol.h>
#include <relative-pointer-unstable-v1-client-protocol.h>
#include <pointer-constraints-unstable-v1-client-protocol.h>
#include <pointer-gestures-unstable-v1-client-protocol.h>
#include <keyboard-shortcuts-inhibit-unstable-v1-client-protocol.h>
#include <ext-idle-notify-v1-client-protocol.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

#include "common_priv.h"
#include "platform_priv.h"
#include "input_priv.h"

extern const uint32_t WSI_WL_SEAT_VERSION;

static void
wsi_pointer_set_cursor_image(
    struct wsi_pointer *ptr,
    struct wl_cursor_image *image,
    uint32_t serial)
{
    struct wl_buffer *buffer = wl_cursor_image_get_buffer(image);

    wl_surface_attach(ptr->wl_surface, buffer, 0, 0);
    wl_surface_commit(ptr->wl_surface);

    wl_pointer_set_cursor(
        ptr->wl_pointer,
        serial,
        ptr->wl_surface,
        (int32_t)image->hotspot_x,
        (int32_t)image->hotspot_y);
}

static void
wsi_pointer_set_cursor(
    struct wsi_pointer *ptr,
    const char *name,
    uint32_t serial)
{
    ptr->wl_cursor = wl_cursor_theme_get_cursor(ptr->wl_cursor_theme, name);
    if (ptr->wl_cursor) {
        wsi_pointer_set_cursor_image(ptr, ptr->wl_cursor->images[0], serial);
        return;
    }

    wl_surface_attach(ptr->wl_surface, NULL, 0, 0);
    wl_surface_commit(ptr->wl_surface);
    wl_pointer_set_cursor(ptr->wl_pointer, serial, NULL, 0, 0);
}

void
wsi_pointer_constraint_destroy(struct wsi_pointer_constraint *cons)
{
    wl_list_remove(&cons->link);

    if (cons->type == ZWP_POINTER_CONSTRAINTS_V1_LOCK_POINTER) {
        assert(cons->wp_locked_pointer_v1 != NULL);
        zwp_locked_pointer_v1_destroy(cons->wp_locked_pointer_v1);
    } else if (cons->type == ZWP_POINTER_CONSTRAINTS_V1_CONFINE_POINTER) {
        assert(cons->wp_confined_pointer_v1 != NULL);
        zwp_confined_pointer_v1_destroy(cons->wp_confined_pointer_v1);
    }

    free(cons);
}

static void
wsi_pointer_frame(struct wsi_pointer *pointer)
{
    if (pointer->frame.mask & WSI_POINTER_FRAME_EVENT_ENTER) {
        wsi_pointer_set_cursor(pointer, "left_ptr", pointer->frame.serial);
    }

    pointer->frame.mask = WSI_POINTER_FRAME_EVENT_NONE;
}

// region Wp Swipe Gesture

static void
wp_pointer_gesture_swipe_v1_begin(
    void *data,
    struct zwp_pointer_gesture_swipe_v1 *wp_pointer_gesture_swipe_v1,
    uint32_t serial,
    uint32_t time,
    struct wl_surface *surface,
    uint32_t fingers)
{
}

static void
wp_pointer_gesture_swipe_v1_update(
    void *data,
    struct zwp_pointer_gesture_swipe_v1 *zwp_pointer_gesture_swipe_v1,
    uint32_t time,
    wl_fixed_t dx,
    wl_fixed_t dy)
{
}

static void
wp_pointer_gesture_swipe_v1_end(
    void *data,
    struct zwp_pointer_gesture_swipe_v1 *zwp_pointer_gesture_swipe_v1,
    uint32_t serial,
    uint32_t time,
    int32_t cancelled)
{
}

static const struct zwp_pointer_gesture_swipe_v1_listener wp_pointer_gesture_swipe_v1_listenter = {
    .begin  = wp_pointer_gesture_swipe_v1_begin,
    .update = wp_pointer_gesture_swipe_v1_update,
    .end    = wp_pointer_gesture_swipe_v1_end,
};

// endregion

// region Wp Pinch Gesture

static void
wp_pointer_gesture_pinch_v1_begin(
    void *data,
    struct zwp_pointer_gesture_pinch_v1 *wp_pointer_gesture_pinch_v1,
    uint32_t serial,
    uint32_t time,
    struct wl_surface *surface,
    uint32_t fingers)
{
}

static void
wp_pointer_gesture_pinch_v1_update(
    void *data,
    struct zwp_pointer_gesture_pinch_v1 *zwp_pointer_gesture_pinch_v1,
    uint32_t time,
    wl_fixed_t dx,
    wl_fixed_t dy,
    wl_fixed_t scale,
    wl_fixed_t rotation)
{
}

static void
wp_pointer_gesture_pinch_v1_end(
    void *data,
    struct zwp_pointer_gesture_pinch_v1 *zwp_pointer_gesture_pinch_v1,
    uint32_t serial,
    uint32_t time,
    int32_t cancelled)
{
}

static const struct zwp_pointer_gesture_pinch_v1_listener wp_pointer_gesture_pinch_v1_listenter = {
    .begin  = wp_pointer_gesture_pinch_v1_begin,
    .update = wp_pointer_gesture_pinch_v1_update,
    .end    = wp_pointer_gesture_pinch_v1_end,
};

// endregion

// region Wp Hold Gesture

static void
wp_pointer_gesture_hold_v1_begin(
    void *data,
    struct zwp_pointer_gesture_hold_v1 *wp_pointer_gesture_hold_v1,
    uint32_t serial,
    uint32_t time,
    struct wl_surface *surface,
    uint32_t fingers)
{
}

static void
wp_pointer_gesture_hold_v1_end(
    void *data,
    struct zwp_pointer_gesture_hold_v1 *zwp_pointer_gesture_hold_v1,
    uint32_t serial,
    uint32_t time,
    int32_t cancelled)
{
}

static const struct zwp_pointer_gesture_hold_v1_listener wp_pointer_gesture_hold_v1_listenter = {
    .begin  = wp_pointer_gesture_hold_v1_begin,
    .end    = wp_pointer_gesture_hold_v1_end,
};

// endregion

static void
wsi_pointer_init_gestures(struct wsi_pointer *pointer)
{
    struct wsi_pointer_gestures *gestures = &pointer->gestures;
    struct wsi_seat *seat = wl_container_of(pointer, seat, pointer);
    struct wsi_platform *platform = seat->global.platform;

    assert(platform->wp_pointer_gestures_v1 != NULL);
    assert(gestures->wp_swipe_v1 == NULL);
    assert(gestures->wp_pinch_v1 == NULL);
    assert(gestures->wp_hold_v1 == NULL);

    gestures->wp_swipe_v1 = zwp_pointer_gestures_v1_get_swipe_gesture(
        platform->wp_pointer_gestures_v1,
        pointer->wl_pointer);
    zwp_pointer_gesture_swipe_v1_add_listener(
        gestures->wp_swipe_v1,
        &wp_pointer_gesture_swipe_v1_listenter,
        gestures);

    gestures->wp_pinch_v1 = zwp_pointer_gestures_v1_get_pinch_gesture(
        platform->wp_pointer_gestures_v1,
        pointer->wl_pointer);
    zwp_pointer_gesture_pinch_v1_add_listener(
        gestures->wp_pinch_v1,
        &wp_pointer_gesture_pinch_v1_listenter,
        gestures);

    if (zwp_pointer_gestures_v1_get_version(platform->wp_pointer_gestures_v1) >= 
        ZWP_POINTER_GESTURES_V1_GET_HOLD_GESTURE_SINCE_VERSION) 
    {
        pointer->gestures.wp_hold_v1 = zwp_pointer_gestures_v1_get_hold_gesture(
            platform->wp_pointer_gestures_v1,
            pointer->wl_pointer);
        zwp_pointer_gesture_hold_v1_add_listener(
            gestures->wp_hold_v1,
            &wp_pointer_gesture_hold_v1_listenter,
            gestures);
    }
}

static void
wsi_pointer_uninit_gestures(struct wsi_pointer *pointer)
{
    struct wsi_pointer_gestures *gestures = &pointer->gestures;

    if (gestures->wp_swipe_v1) {
        zwp_pointer_gesture_swipe_v1_destroy(gestures->wp_swipe_v1);
    }

    if (gestures->wp_pinch_v1) {
        zwp_pointer_gesture_pinch_v1_destroy(gestures->wp_pinch_v1);
    }

    if (gestures->wp_hold_v1) {
        zwp_pointer_gesture_hold_v1_destroy(gestures->wp_hold_v1);
    }

    memset(gestures, 0, sizeof(struct wsi_pointer_gestures));
}

// region Wp Locked Pointer

static void
wp_pointer_locked(void *data, struct zwp_locked_pointer_v1 *wp_locked_pointer_v1)
{
    struct wsi_pointer_constraint *cons = data;
    struct wsi_pointer *pointer = cons->pointer;

    pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_STATE;
    pointer->frame.state = WSI_POINTER_STATE_LOCKED;

    wsi_pointer_frame(pointer);
}

static void
wp_pointer_unlocked(void *data, struct zwp_locked_pointer_v1 *wp_locked_pointer_v1)
{
    struct wsi_pointer_constraint *cons = data;
    struct wsi_pointer *pointer = cons->pointer;

    pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_STATE;
    pointer->frame.state = WSI_POINTER_STATE_NONE;

    if (cons->lifetime == ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT) {
        wsi_pointer_constraint_destroy(cons);
    }

    wsi_pointer_frame(pointer);
}

static const struct zwp_locked_pointer_v1_listener wp_locked_pointer_v1_listener = {
    .locked = wp_pointer_locked,
    .unlocked = wp_pointer_unlocked,
};

// endregion

// region Wp Confined Pointer

static void
wp_pointer_confined(void *data, struct zwp_confined_pointer_v1 *wp_confined_pointer_v1)
{
    struct wsi_pointer_constraint *cons = data;
    struct wsi_pointer *pointer = cons->pointer;

    pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_STATE;
    pointer->frame.state = WSI_POINTER_STATE_CONFINED;

    wsi_pointer_frame(pointer);
}

static void
wp_pointer_unconfined(void *data, struct zwp_confined_pointer_v1 *wp_confined_pointer_v1)
{
    struct wsi_pointer_constraint *cons = data;
    struct wsi_pointer *pointer = cons->pointer;

    pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_STATE;
    pointer->frame.state = WSI_POINTER_STATE_NONE;

    if (cons->lifetime == ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT) {
        wsi_pointer_constraint_destroy(cons);
    }

    wsi_pointer_frame(pointer);
}

static const struct zwp_confined_pointer_v1_listener wp_confined_pointer_v1_listener = {
    .confined = wp_pointer_confined,
    .unconfined = wp_pointer_unconfined,
};

// endregion

// region Wp Relative Pointer

static void
wp_pointer_relative_motion(
    void *data,
    struct zwp_relative_pointer_v1 *wp_relative_pointer_v1,
    uint32_t utime_hi,
    uint32_t utime_lo,
    wl_fixed_t dx,
    wl_fixed_t dy,
    wl_fixed_t dx_unaccel,
    wl_fixed_t dy_unaccel)
{
    struct wsi_pointer *pointer = data;

    // TODO: The spec doesn't clarify if wp_input_timestamps apply to this event.
    //  Weston doesn't send a timestamp event, so for now just use this value.
    //  There is also a small bug here where if another event follows this one
    //  in the same frame e.g. wl_pointer.motion then the lower resolution timestamp
    //  replaces this one.

    pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_MOTION_RELATIVE;
    pointer->frame.time = wsi_us_to_ns(utime_hi, utime_lo);
    pointer->frame.dx = wl_fixed_to_double(dx);
    pointer->frame.dy = wl_fixed_to_double(dy);
    pointer->frame.udx = wl_fixed_to_double(dx_unaccel);
    pointer->frame.udy = wl_fixed_to_double(dy_unaccel);

    if (wl_pointer_get_version(pointer->wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
        wsi_pointer_frame(pointer);
    }
}

static const struct zwp_relative_pointer_v1_listener wp_relative_pointer_v1_listener = {
    .relative_motion = wp_pointer_relative_motion,
};

// endregion

// region Wp Pointer Timestamp

static void
wp_pointer_timestamp(
    void *data,
    struct zwp_input_timestamps_v1 *wp_input_timestamps_v1,
    uint32_t tv_sec_hi,
    uint32_t tv_sec_lo,
    uint32_t tv_nsec)
{
    struct wsi_pointer *pointer = data;
    pointer->frame.time = wsi_tv_to_ns(tv_sec_hi, tv_sec_lo, tv_nsec);
}

static const struct zwp_input_timestamps_v1_listener wp_pointer_timestamps_v1_listener = {
    .timestamp = wp_pointer_timestamp,
};

// endregion

// region Wl Pointer

static void
wl_pointer_enter(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial,
    struct wl_surface *surface,
    wl_fixed_t sx,
    wl_fixed_t sy)
{
    struct wsi_pointer *pointer = data;

    pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_ENTER;
    pointer->frame.serial = serial;
    pointer->frame.enter = surface;
    pointer->frame.x = wl_fixed_to_double(sx);
    pointer->frame.y = wl_fixed_to_double(sy);

    if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
        wsi_pointer_frame(pointer);
    }
}

static void
wl_pointer_leave(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial,
    struct wl_surface *surface)
{
    struct wsi_pointer *pointer = data;

    pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_LEAVE;
    pointer->frame.serial = serial;
    pointer->frame.leave = surface;

    if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
        wsi_pointer_frame(pointer);
    }
}

static void
wl_pointer_motion(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t time,
    wl_fixed_t sx,
    wl_fixed_t sy)
{
    struct wsi_pointer *pointer = data;

    pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_MOTION_ABSOLUTE;
    if (pointer->wp_timestamps_v1 == NULL) {
        pointer->frame.time = wsi_ms_to_ns(time);
    }
    pointer->frame.x = wl_fixed_to_double(sx);
    pointer->frame.y = wl_fixed_to_double(sy);

    if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
        wsi_pointer_frame(pointer);
    }
}

static void
wl_pointer_button(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state)
{
    struct wsi_pointer *pointer = data;

    pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_BUTTON;
    pointer->frame.serial = serial;
    if (pointer->wp_timestamps_v1 == NULL) {
        pointer->frame.time = wsi_ms_to_ns(time);
    }
    pointer->frame.button.code = button;
    pointer->frame.button.state = state;

    if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
        wsi_pointer_frame(pointer);
    }
}

static void
wl_pointer_axis(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t time,
    uint32_t axis,
    wl_fixed_t value)
{
    struct wsi_pointer *pointer = data;

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
        pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_AXIS_X;
        pointer->frame.axis.x.value = wl_fixed_to_double(value);
    } else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_AXIS_Y;
        pointer->frame.axis.y.value = wl_fixed_to_double(value);
    } else {
        return;
    }

    if (pointer->wp_timestamps_v1 == NULL) {
        pointer->frame.time = wsi_ms_to_ns(time);
    }

    if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
        wsi_pointer_frame(pointer);
    }
}

static void
wl_pointer_frame(
    void *data,
    struct wl_pointer *wl_pointer)
{
    struct wsi_pointer *pointer = data;

    wsi_pointer_frame(pointer);
}

static void
wl_pointer_axis_source(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t axis_source)
{
    struct wsi_pointer *pointer = data;

    pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_AXIS_SOURCE;
    pointer->frame.axis.source = axis_source;
}

static void
wl_pointer_axis_stop(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t time,
    uint32_t axis)
{
    struct wsi_pointer *pointer = data;

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
        pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_AXIS_X_STOP;
    } else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_AXIS_Y_STOP;
    } else {
        return;
    }

    if (pointer->wp_timestamps_v1 == NULL) {
        pointer->frame.time = wsi_ms_to_ns(time);
    }
}

static void
wl_pointer_axis_discrete(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t axis,
    int32_t discrete)
{
    struct wsi_pointer *pointer = data;

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
        pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_AXIS_X_DISCRETE;
        pointer->frame.axis.x.discrete = discrete;
    } else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_AXIS_Y_DISCRETE;
        pointer->frame.axis.y.discrete = discrete;
    }
}

#ifdef WL_POINTER_AXIS_VALUE120_SINCE_VERSION
static void
wl_pointer_axis_value120(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t axis,
    int32_t value)
{
    struct wsi_pointer *pointer = data;

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
        pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_AXIS_X_DISCRETE;
        pointer->frame.axis.x.discrete = value / 120.0;
    } else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_AXIS_Y_DISCRETE;
        pointer->frame.axis.y.discrete = value / 120.0;
    }
}
#endif

#ifdef WL_POINTER_AXIS_RELATIVE_DIRECTION_SINCE_VERSION
static void
wl_pointer_axis_relative_direction(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t axis,
    uint32_t direction)
{
    struct wsi_pointer *pointer = data;

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
        pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_AXIS_X_DIRECTION;
        pointer->frame.axis.x.direction = direction;
    } else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        pointer->frame.mask |= WSI_POINTER_FRAME_EVENT_AXIS_Y_DIRECTION;
        pointer->frame.axis.y.direction = direction;
    }
}
#endif

static const struct wl_pointer_listener wl_pointer_listener = {
    .enter = wl_pointer_enter,
    .leave = wl_pointer_leave,
    .motion = wl_pointer_motion,
    .button = wl_pointer_button,
    .axis = wl_pointer_axis,
    .frame = wl_pointer_frame,
    .axis_source = wl_pointer_axis_source,
    .axis_stop = wl_pointer_axis_stop,
    .axis_discrete = wl_pointer_axis_discrete,
#ifdef WL_POINTER_AXIS_VALUE120_SINCE_VERSION
    .axis_value120 = wl_pointer_axis_value120,
#endif
#ifdef WL_POINTER_AXIS_RELATIVE_DIRECTION_SINCE_VERSION
    .axis_relative_direction = wl_pointer_axis_relative_direction,
#endif
};

// endregion

static struct wsi_pointer_constraint *
wsi_pointer_find_constraint(struct wsi_pointer *pointer, struct wl_surface *wl_surface)
{
    struct wsi_pointer_constraint *cons;
    wl_list_for_each(cons, &pointer->constraints, link) {
        if (cons->wl_surface == wl_surface) {
            return cons;
        }
    }
    return NULL;
}

void
wsi_pointer_constrain(
    struct wsi_pointer *pointer,
    uint32_t type,
    struct wl_surface *wl_surface,
    bool persistent,
    double pos_hint[2])
{
    struct wsi_seat *seat = wl_container_of(pointer, seat, pointer);
    struct wsi_platform *platform = seat->global.platform;

    assert(platform->wp_pointer_constraints_v1 != NULL);
    assert(pointer->wl_pointer != NULL);
    assert(type == ZWP_POINTER_CONSTRAINTS_V1_LOCK_POINTER ||
           type == ZWP_POINTER_CONSTRAINTS_V1_CONFINE_POINTER);

    struct wsi_pointer_constraint *cons = wsi_pointer_find_constraint(pointer, wl_surface);
    if (cons) {
        return;
    }

    cons = calloc(1, sizeof(struct wsi_pointer_constraint));
    if (!cons) {
        return;
    }

    cons->pointer = pointer;
    cons->wl_surface = wl_surface;
    cons->type = type;
    cons->lifetime = persistent
                   ? ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT
                   : ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT;

    if (type == ZWP_POINTER_CONSTRAINTS_V1_LOCK_POINTER) {
        cons->wp_locked_pointer_v1 = zwp_pointer_constraints_v1_lock_pointer(
            platform->wp_pointer_constraints_v1,
            wl_surface,
            pointer->wl_pointer,
            NULL,
            cons->lifetime);
        if (!cons->wp_locked_pointer_v1) {
            free(cons);
            return;
        }

        zwp_locked_pointer_v1_add_listener(
            cons->wp_locked_pointer_v1,
            &wp_locked_pointer_v1_listener,
            cons);

        if (pos_hint) {
            zwp_locked_pointer_v1_set_cursor_position_hint(
                cons->wp_locked_pointer_v1,
                wl_fixed_from_double(pos_hint[0]),
                wl_fixed_from_double(pos_hint[1]));
        }
    }
    else if (type == ZWP_POINTER_CONSTRAINTS_V1_CONFINE_POINTER) {
        cons->wp_confined_pointer_v1 = zwp_pointer_constraints_v1_confine_pointer(
            platform->wp_pointer_constraints_v1,
            wl_surface,
            pointer->wl_pointer,
            NULL,
            cons->lifetime);
        if (!cons->wp_confined_pointer_v1) {
            free(cons);
            return;
        }

        zwp_confined_pointer_v1_add_listener(
            cons->wp_confined_pointer_v1,
            &wp_confined_pointer_v1_listener,
            cons);
    }

    wl_list_insert(&pointer->constraints, &cons->link);
}

void
wsi_pointer_remove_constraint(struct wsi_pointer *pointer, struct wl_surface *wl_surface)
{
    struct wsi_pointer_constraint *cons = wsi_pointer_find_constraint(pointer, wl_surface);
    if (cons == NULL) {
        return;
    }

    wsi_pointer_constraint_destroy(cons);
}

static bool
wsi_seat_init_pointer(struct wsi_seat *seat)
{
    assert(seat->pointer.wl_pointer == NULL);

    struct wsi_platform *plat = seat->global.platform;
    wl_list_init(&seat->pointer.constraints);

    seat->pointer.wl_pointer = wl_seat_get_pointer(seat->wl_seat);
    wl_pointer_add_listener(seat->pointer.wl_pointer, &wl_pointer_listener, &seat->pointer);

    seat->pointer.wl_cursor_theme = wl_cursor_theme_load(NULL, 24, plat->wl_shm);
    seat->pointer.wl_surface = wl_compositor_create_surface(plat->wl_compositor);

    if (plat->wp_input_timestamps_manager_v1) {
        seat->pointer.wp_timestamps_v1 = zwp_input_timestamps_manager_v1_get_pointer_timestamps(
            plat->wp_input_timestamps_manager_v1,
            seat->pointer.wl_pointer);
        zwp_input_timestamps_v1_add_listener(
            seat->pointer.wp_timestamps_v1,
            &wp_pointer_timestamps_v1_listener,
            &seat->pointer);
    }

    if (plat->wp_relative_pointer_manager_v1) {
        seat->pointer.wp_relative_v1 = zwp_relative_pointer_manager_v1_get_relative_pointer(
            plat->wp_relative_pointer_manager_v1,
            seat->pointer.wl_pointer);
        zwp_relative_pointer_v1_add_listener(
            seat->pointer.wp_relative_v1,
            &wp_relative_pointer_v1_listener,
            &seat->pointer);
    }

    if (plat->wp_pointer_gestures_v1) {
        wsi_pointer_init_gestures(&seat->pointer);
    }

    return true;
}

static void
wsi_seat_uninit_pointer(struct wsi_seat *seat)
{
    struct wsi_pointer *pointer = &seat->pointer;
    assert(pointer->wl_pointer != NULL);

    struct wsi_pointer_constraint *cons, *tmp;
    wl_list_for_each_safe(cons, tmp, &pointer->constraints, link) {
        wsi_pointer_constraint_destroy(cons);
    }

    wsi_pointer_uninit_gestures(pointer);

    if (pointer->wp_timestamps_v1) {
        zwp_input_timestamps_v1_destroy(pointer->wp_timestamps_v1);
    }

    if (pointer->wp_relative_v1) {
        zwp_relative_pointer_v1_destroy(pointer->wp_relative_v1);
    }

    if (wl_pointer_get_version(pointer->wl_pointer) >= WL_POINTER_RELEASE_SINCE_VERSION) {
        wl_pointer_release(pointer->wl_pointer);
    } else {
        wl_pointer_destroy(pointer->wl_pointer);
    }

    wl_cursor_theme_destroy(pointer->wl_cursor_theme);
    wl_surface_destroy(pointer->wl_surface);

    memset(pointer, 0, sizeof(struct wsi_pointer));
}

static const char *
wsi_keyboard_get_locale()
{
    const char *locale = getenv("LC_ALL");
    if (!locale || locale[0] == '\0') {
        locale = getenv("LC_CTYPE");
    }
    if (!locale || locale[0] == '\0') {
        locale = getenv("LANG");
    }
    if (!locale || locale[0] == '\0') {
        locale = "C";
    }
    return locale;
}

// region Wp Keyboard Timestamp

static void
wp_keyboard_timestamp(
    void *data,
    struct zwp_input_timestamps_v1 *wp_input_timestamps_v1,
    uint32_t tv_sec_hi,
    uint32_t tv_sec_lo,
    uint32_t tv_nsec)
{
    struct wsi_keyboard *keyboard = data;
    keyboard->event_time = wsi_tv_to_ns(tv_sec_hi, tv_sec_lo, tv_nsec);
}

static const struct zwp_input_timestamps_v1_listener wp_keyboard_timestamps_v1_listener = {
    .timestamp = wp_keyboard_timestamp,
};

// endregion

// region Wl Keyboard

static void
wl_keyboard_keymap(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32_t format,
    int32_t fd,
    uint32_t size)
{
    struct wsi_keyboard *keyboard = data;

    xkb_compose_state_unref(keyboard->xkb_compose_state);
    keyboard->xkb_compose_state = NULL;

    xkb_state_unref(keyboard->xkb_state);
    keyboard->xkb_state = NULL;

    xkb_keymap_unref(keyboard->xkb_keymap);
    keyboard->xkb_keymap = NULL;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    char *map_str = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (map_str == MAP_FAILED) {
        return;
    }

    struct xkb_keymap *keymap = xkb_keymap_new_from_string(
        keyboard->xkb_context,
        map_str,
        XKB_KEYMAP_FORMAT_TEXT_V1,
        XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_str, size);
    if (!keymap) {
        return;
    }

    struct xkb_state *state = xkb_state_new(keymap);
    if (!state) {
        xkb_keymap_unref(keymap);
        return;
    }

    const char *locale = wsi_keyboard_get_locale();

    struct xkb_compose_state *compose_state = NULL;
    struct xkb_compose_table *table = xkb_compose_table_new_from_locale(
        keyboard->xkb_context,
        locale,
        XKB_COMPOSE_COMPILE_NO_FLAGS);
    if (table) {
        compose_state = xkb_compose_state_new(table, XKB_COMPOSE_STATE_NO_FLAGS);
        xkb_compose_table_unref(table);
    }

    keyboard->xkb_compose_state = compose_state;
    keyboard->xkb_keymap = keymap;
    keyboard->xkb_state = state;
}

static void
wl_keyboard_enter(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32_t serial,
    struct wl_surface *surface,
    struct wl_array *keys)
{
}

static void
wl_keyboard_leave(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32_t serial,
    struct wl_surface *surface)
{
}

static void
wl_keyboard_key(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32_t serial,
    uint32_t time,
    uint32_t key,
    uint32_t state)
{
}

static void
wl_keyboard_modifiers(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32_t serial,
    uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group)
{
    struct wsi_keyboard *keyboard = data;

    if (keyboard->xkb_state) {
        xkb_state_update_mask(
            keyboard->xkb_state,
            mods_depressed,
            mods_latched,
            mods_locked,
            0,
            0,
            group);
    }
}

static void
wl_keyboard_repeat_info(
    void *data,
    struct wl_keyboard *wl_keyboard,
    int32_t rate,
    int32_t delay)
{
    struct wsi_keyboard *keyboard = data;

    keyboard->repeat_rate = rate;
    keyboard->repeat_delay = delay;
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap = wl_keyboard_keymap,
    .enter = wl_keyboard_enter,
    .leave = wl_keyboard_leave,
    .key = wl_keyboard_key,
    .modifiers = wl_keyboard_modifiers,
    .repeat_info = wl_keyboard_repeat_info,
};

// endregion

static bool
wsi_seat_init_keyboard(struct wsi_seat *seat)
{
    assert(seat->keyboard.wl_keyboard == NULL);

    struct wsi_platform *plat = seat->global.platform;

    seat->keyboard.xkb_context = xkb_context_ref(plat->xkb_context);

    seat->keyboard.wl_keyboard = wl_seat_get_keyboard(seat->wl_seat);
    wl_keyboard_add_listener(seat->keyboard.wl_keyboard, &wl_keyboard_listener, &seat->keyboard);

    if (plat->wp_input_timestamps_manager_v1) {
        seat->keyboard.wp_timestamps_v1 = zwp_input_timestamps_manager_v1_get_keyboard_timestamps(
            plat->wp_input_timestamps_manager_v1,
            seat->keyboard.wl_keyboard);
        zwp_input_timestamps_v1_add_listener(
            seat->keyboard.wp_timestamps_v1,
            &wp_keyboard_timestamps_v1_listener,
            &seat->keyboard);
    }

    seat->keyboard.event_time = -1;
    return true;
}

static void
wsi_seat_uninit_keyboard(struct wsi_seat *seat)
{
    struct wsi_keyboard *keyboard = &seat->keyboard;
    assert(keyboard->wl_keyboard != NULL);

    if (keyboard->wp_timestamps_v1) {
        zwp_input_timestamps_v1_destroy(keyboard->wp_timestamps_v1);
    }

    if (wl_keyboard_get_version(keyboard->wl_keyboard) >= WL_KEYBOARD_RELEASE_SINCE_VERSION) {
        wl_keyboard_release(keyboard->wl_keyboard);
    } else {
        wl_keyboard_destroy(keyboard->wl_keyboard);
    }

    xkb_compose_state_unref(keyboard->xkb_compose_state);
    xkb_state_unref(keyboard->xkb_state);
    xkb_keymap_unref(keyboard->xkb_keymap);
    xkb_context_unref(keyboard->xkb_context);

    memset(keyboard, 0, sizeof(struct wsi_keyboard));
}

// region Wp Keyboard Shortcuts Inhibitor

static void
wp_keyboard_shortcuts_inhibitor_v1_active(
    void *data,
    struct zwp_keyboard_shortcuts_inhibitor_v1 *inhibitor)
{
}

static void
wp_keyboard_shortcuts_inhibitor_v1_inactive(
    void *data,
    struct zwp_keyboard_shortcuts_inhibitor_v1 *inhibitor)
{
}

static const struct zwp_keyboard_shortcuts_inhibitor_v1_listener wp_keyboard_shortcuts_inhibitor_v1_listener = {
    .active = wp_keyboard_shortcuts_inhibitor_v1_active,
    .inactive = wp_keyboard_shortcuts_inhibitor_v1_inactive,
};

// endregion

// region Ext Idle Notification

static void
ext_idle_notification_v1_idled(void *data, struct ext_idle_notification_v1 *notification)
{
}

static void
ext_idle_notification_v1_resumed(void *data, struct ext_idle_notification_v1 *notification)
{
}

static const struct ext_idle_notification_v1_listener ext_idle_notification_v1_listener = {
    .idled = ext_idle_notification_v1_idled,
    .resumed = ext_idle_notification_v1_resumed,
};

// endregion

// region WL Seat

static void
wl_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities)
{
    struct wsi_seat *seat = data;

    seat->capabilities = capabilities;

    bool has_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
    bool has_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

    if (has_pointer && seat->pointer.wl_pointer == NULL) {
        wsi_seat_init_pointer(seat);
    } else if (!has_pointer && seat->pointer.wl_pointer != NULL) {
        wsi_seat_uninit_pointer(seat);
    }

    if (has_keyboard && seat->keyboard.wl_keyboard == NULL) {
        wsi_seat_init_keyboard(seat);
    } else if (!has_keyboard && seat->keyboard.wl_keyboard != NULL) {
        wsi_seat_uninit_keyboard(seat);
    }
}

static void
wl_seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{
    struct wsi_seat *seat = data;

    free(seat->name);
    seat->name = strdup(name);
}

static const struct wl_seat_listener wl_seat_listener = {
    .capabilities = wl_seat_capabilities,
    .name         = wl_seat_name,
};

// endregion

static struct wsi_shortcuts_inhibitor *
wsi_seat_find_shortcuts_inhibitor(struct wsi_seat *seat, struct wl_surface *wl_surface)
{
    struct wsi_shortcuts_inhibitor *inhibitor;
    wl_list_for_each(inhibitor, &seat->shortcut_inhibitors, link) {
        if (inhibitor->wl_surface == wl_surface) {
            return inhibitor;
        }
    }
    return NULL;
}

void
wsi_seat_inhibit_shortcuts(struct wsi_seat *seat, struct wl_surface *wl_surface)
{
    struct wsi_platform *platform = seat->global.platform;
    assert(platform->wp_keyboard_shortcuts_inhibit_manager_v1 != NULL);

    struct wsi_shortcuts_inhibitor *inhibitor = wsi_seat_find_shortcuts_inhibitor(seat, wl_surface);
    if (inhibitor) {
        return;
    }

    inhibitor = calloc(1, sizeof(struct wsi_shortcuts_inhibitor));
    if (!inhibitor) {
        return;
    }

    inhibitor->seat = seat;
    inhibitor->wl_surface = wl_surface;
    inhibitor->wp_shortcuts_inhibitor_v1 = zwp_keyboard_shortcuts_inhibit_manager_v1_inhibit_shortcuts(
        platform->wp_keyboard_shortcuts_inhibit_manager_v1,
        wl_surface,
        seat->wl_seat);
    if (!inhibitor->wp_shortcuts_inhibitor_v1) {
        free(inhibitor);
        return;
    }

    zwp_keyboard_shortcuts_inhibitor_v1_add_listener(
        inhibitor->wp_shortcuts_inhibitor_v1,
        &wp_keyboard_shortcuts_inhibitor_v1_listener,
        inhibitor);

    wl_list_insert(&seat->shortcut_inhibitors, &inhibitor->link);
}

void
wsi_seat_restore_shortcuts(struct wsi_seat *seat, struct wl_surface *wl_surface)
{
    struct wsi_shortcuts_inhibitor *inhibitor = wsi_seat_find_shortcuts_inhibitor(seat, wl_surface);
    if (!inhibitor) {
        return;
    }

    wl_list_remove(&inhibitor->link);
    zwp_keyboard_shortcuts_inhibitor_v1_destroy(inhibitor->wp_shortcuts_inhibitor_v1);
    free(inhibitor);
}

void
wsi_seat_set_idle_timer(struct wsi_seat *seat, const uint32_t *time)
{
    struct wsi_platform *platform = seat->global.platform;
    assert(platform->ext_idle_notifier_v1 != NULL);

    if (seat->ext_idle_notification_v1) {
        ext_idle_notification_v1_destroy(seat->ext_idle_notification_v1);
        seat->ext_idle_notification_v1 = NULL;
    }

    if (!time) {
        return;
    }

    seat->ext_idle_notification_v1 = ext_idle_notifier_v1_get_idle_notification(
        platform->ext_idle_notifier_v1,
        *time,
        seat->wl_seat);
    ext_idle_notification_v1_add_listener(
        seat->ext_idle_notification_v1,
        &ext_idle_notification_v1_listener,
        seat);
}

static bool
wsi_seat_init(struct wsi_seat *seat)
{
    assert(seat->wl_seat == NULL);
    assert(seat->global.name != 0);

    struct wsi_platform *plat = seat->global.platform;
    wl_list_init(&seat->shortcut_inhibitors);

    uint32_t version = wsi_get_version(
        &wl_seat_interface,
        seat->global.version,
        WSI_WL_SEAT_VERSION);

    seat->wl_seat = wl_registry_bind(
        plat->wl_registry,
        seat->global.name,
        &wl_seat_interface,
        version);
    if (seat->wl_seat == NULL) {
        return false;
    }

    wl_seat_add_listener(seat->wl_seat, &wl_seat_listener, seat);
    return true;
}

static void
wsi_seat_uninit(struct wsi_seat *seat)
{
    assert(seat->wl_seat != NULL);

    struct wsi_shortcuts_inhibitor *inhibitor, *tmp;
    wl_list_for_each_safe(inhibitor, tmp, &seat->shortcut_inhibitors, link) {
        zwp_keyboard_shortcuts_inhibitor_v1_destroy(inhibitor->wp_shortcuts_inhibitor_v1);
        wl_list_remove(&inhibitor->link);
        free(inhibitor);
    }

    if (seat->ext_idle_notification_v1) {
        ext_idle_notification_v1_destroy(seat->ext_idle_notification_v1);
        seat->ext_idle_notification_v1 = NULL;
    }

    if (seat->pointer.wl_pointer) {
        wsi_seat_uninit_pointer(seat);
    }

    if (seat->keyboard.wl_keyboard) {
        wsi_seat_uninit_keyboard(seat);
    }

    free(seat->name);
    seat->name = NULL;
    seat->capabilities = 0;

    if (wl_seat_get_version(seat->wl_seat) >= WL_SEAT_RELEASE_SINCE_VERSION) {
        wl_seat_release(seat->wl_seat);
    } else {
        wl_seat_destroy(seat->wl_seat);
    }
    seat->wl_seat = NULL;
}

static struct wsi_seat *
wsi_seat_find(struct wsi_platform *platform, uint64_t id)
{
    struct wsi_seat *seat;
    wl_list_for_each(seat, &platform->seat_list, link) {
        if (seat->global.id == id) {
            return seat;
        }
    }
    return NULL;
}

struct wsi_seat *
wsi_seat_add(struct wsi_platform *platform, uint32_t name, uint32_t version)
{
    struct wsi_seat *seat = calloc(1, sizeof(struct wsi_seat));
    if (!seat) {
        return NULL;
    }

    seat->global.platform = platform;
    seat->global.id = wsi_new_id(platform);
    seat->global.name = name;
    seat->global.version = version;

    wl_list_insert(&platform->seat_list, &seat->link);
    return seat;
}

void
wsi_seat_remove(struct wsi_seat *seat)
{
    assert(seat->global.name != 0);

    seat->global.name = 0;
    seat->global.version = 0;

    wl_list_remove(&seat->link);

    if (seat->wl_seat != NULL) {
        wsi_seat_uninit(seat);
    } else {
        free(seat);
    }
}

void
wsi_seat_destroy_all(struct wsi_platform *platform)
{
    struct wsi_seat *seat, *tmp;
    wl_list_for_each_safe(seat, tmp, &platform->seat_list, link) {
        wl_list_remove(&seat->link);

        if (seat->wl_seat != NULL) {
            wsi_seat_uninit(seat);
        }

        free(seat);
    }
}

WsiResult
wsiEnumerateSeats(WsiPlatform platform, uint32_t *pIdCount, uint64_t *pIds)
{
    assert(pIdCount != NULL);

    if (pIds == NULL) {
        *pIdCount = wl_list_length(&platform->seat_list);
        return WSI_SUCCESS;
    }

    uint32_t count = 0;
    struct wsi_seat *seat;
    wl_list_for_each_reverse(seat, &platform->seat_list, link) {
        if (count >= *pIdCount) {
            return WSI_INCOMPLETE;
        }
        pIds[count++] = seat->global.id;
    }

    *pIdCount = count;
    return WSI_SUCCESS;
}

WsiResult
wsiAcquireSeat(WsiPlatform platform, const WsiAcquireSeatInfo *pAcquireInfo, WsiSeat *pSeat)
{
    assert(pAcquireInfo->sType == WSI_STRUCTURE_TYPE_ACQUIRE_SEAT_INFO);

    struct wsi_seat *seat = wsi_seat_find(platform, pAcquireInfo->id);
    if (seat == NULL) {
        return WSI_ERROR_SEAT_LOST;
    }

    if (seat->wl_seat == NULL) {
        wsi_seat_init(seat);
    }

    *pSeat = seat;
    return WSI_SUCCESS;
}

void
wsiReleaseSeat(WsiSeat seat)
{
    if (seat->wl_seat != NULL) {
        wsi_seat_uninit(seat);
    } else if (seat->global.name == 0) {
        free(seat);
    } else {
        assert(false);
    }
}
