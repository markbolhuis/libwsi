#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <wayland-client-protocol.h>
#include <wayland-cursor.h>

#include "platform_priv.h"
#include "seat_priv.h"
#include "pointer_priv.h"

static void
wsi_pointer_set_cursor(struct wsi_pointer *pointer, uint32_t serial)
{
    struct wl_cursor_image *image = pointer->wl_cursor->images[0];

    wl_surface_attach(
        pointer->wl_cursor_surface,
        wl_cursor_image_get_buffer(image),
        0, 0);

    wl_surface_commit(pointer->wl_cursor_surface);

    wl_pointer_set_cursor(
        pointer->wl_pointer,
        serial,
        pointer->wl_cursor_surface,
        (int32_t)image->hotspot_x,
        (int32_t)image->hotspot_y);
}

static void
wsi_pointer_frame(struct wsi_pointer *pointer)
{
    if (pointer->frame.mask & WSI_POINTER_EVENT_ENTER) {
        wsi_pointer_set_cursor(pointer, pointer->frame.serial);
    }

    pointer->frame.mask = WSI_POINTER_EVENT_NONE;
}

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

    pointer->frame.mask |= WSI_POINTER_EVENT_ENTER;
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

    pointer->frame.mask |= WSI_POINTER_EVENT_LEAVE;
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

    pointer->frame.mask |= WSI_POINTER_EVENT_MOTION;
    pointer->frame.time = time;
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

    pointer->frame.mask |= WSI_POINTER_EVENT_BUTTON;
    pointer->frame.serial = serial;
    pointer->frame.time = time;
    pointer->frame.button = button;
    pointer->frame.state = state;

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

    pointer->frame.mask |= WSI_POINTER_EVENT_AXIS;
    pointer->frame.axes[axis].start_time = time;
    pointer->frame.axes[axis].value = wl_fixed_to_double(value);

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

    pointer->frame.mask |= WSI_POINTER_EVENT_AXIS_SOURCE;
    pointer->frame.axis_source = axis_source;
}

static void
wl_pointer_axis_stop(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t time,
    uint32_t axis)
{
    struct wsi_pointer *pointer = data;

    pointer->frame.mask |= WSI_POINTER_EVENT_AXIS_STOP;
    pointer->frame.axes[axis].stop_time = time;
}

static void
wl_pointer_axis_discrete(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t axis,
    int32_t discrete)
{
    if (wl_pointer_get_version(wl_pointer) <
        WL_POINTER_AXIS_VALUE120_SINCE_VERSION)
    {
        struct wsi_pointer *pointer = data;

        pointer->frame.mask |= WSI_POINTER_EVENT_AXIS_DISCRETE;
        pointer->frame.axes[axis].discrete = discrete * 120;
    }
}

static void
wl_pointer_axis_value120(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t axis,
    int32_t value)
{
    struct wsi_pointer *pointer = data;

    pointer->frame.mask |= WSI_POINTER_EVENT_AXIS_DISCRETE;
    pointer->frame.axes[axis].discrete = value;
}

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
    .axis_value120 = wl_pointer_axis_value120,
};

// endregion

bool
wsi_pointer_create(struct wsi_seat *seat)
{
    assert(seat->pointer == NULL);

    struct wsi_pointer *ptr = calloc(1, sizeof(struct wsi_pointer));
    if (ptr == NULL) {
        return false;
    }

    struct wsi_platform *plat = seat->global.platform;

    ptr->seat = seat;

    ptr->wl_pointer = wl_seat_get_pointer(seat->wl_seat);
    wl_pointer_add_listener(ptr->wl_pointer, &wl_pointer_listener, ptr);

    ptr->wl_cursor_theme = wl_cursor_theme_load(NULL, 24, plat->wl_shm);
    ptr->wl_cursor_surface = wl_compositor_create_surface(plat->wl_compositor);
    ptr->wl_cursor = wl_cursor_theme_get_cursor(ptr->wl_cursor_theme, "left_ptr");

    seat->pointer = ptr;

    return true;
}

void
wsi_pointer_destroy(struct wsi_seat *seat)
{
    assert(seat->pointer != NULL);

    struct wsi_pointer *ptr = seat->pointer;
    seat->pointer = NULL;

    if (wl_pointer_get_version(ptr->wl_pointer) >=
        WL_POINTER_RELEASE_SINCE_VERSION)
    {
        wl_pointer_release(ptr->wl_pointer);
    } else {
        wl_pointer_destroy(ptr->wl_pointer);
    }

    wl_cursor_theme_destroy(ptr->wl_cursor_theme);
    wl_surface_destroy(ptr->wl_cursor_surface);

    free(ptr);
}
