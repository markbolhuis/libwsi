#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client-protocol.h>
#include <wayland-cursor.h>

#include <xkbcommon/xkbcommon.h>

#include "wsi/input.h"

#include "platform_priv.h"
#include "input_priv.h"

#define WSI_WL_SEAT_VERSION 7

static struct wsi_seat *
wsi_seat_find(struct wsi_platform *platform, uint64_t id)
{
    struct wsi_seat *seat;
    wl_list_for_each(seat, &platform->seat_list, link) {
        if (seat->id == id) {
            return seat;
        }
    }
    return NULL;
}

static void
wsi_pointer_set_cursor(
    struct wsi_pointer *ptr,
    const char *name,
    uint32_t serial)
{
    ptr->wl_cursor = wl_cursor_theme_get_cursor(ptr->wl_cursor_theme, name);

    struct wl_buffer *buffer = NULL;
    int32_t x = 0, y = 0;
    if (ptr->wl_cursor != NULL) {
        struct wl_cursor_image *image = ptr->wl_cursor->images[0];
        buffer = wl_cursor_image_get_buffer(image);
        x = (int32_t)image->hotspot_x;
        y = (int32_t)image->hotspot_y;
    }

    wl_surface_attach(ptr->wl_cursor_surface, buffer, 0, 0);
    wl_surface_commit(ptr->wl_cursor_surface);

    struct wl_surface *surface = ptr->wl_cursor
                               ? ptr->wl_cursor_surface
                               : NULL;
    wl_pointer_set_cursor(ptr->wl_pointer, serial, surface, x, y);
}

static void
wsi_pointer_frame(struct wsi_pointer *pointer)
{
    if (pointer->frame.mask & WSI_WL_POINTER_EVENT_ENTER) {
        wsi_pointer_set_cursor(pointer, "left_ptr", pointer->frame.serial);
    }

    pointer->frame.mask = WSI_WL_POINTER_EVENT_NONE;
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

    pointer->frame.mask |= WSI_WL_POINTER_EVENT_ENTER;
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

    pointer->frame.mask |= WSI_WL_POINTER_EVENT_LEAVE;
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

    pointer->frame.mask |= WSI_WL_POINTER_EVENT_MOTION;
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

    pointer->frame.mask |= WSI_WL_POINTER_EVENT_BUTTON;
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

    pointer->frame.mask |= WSI_WL_POINTER_EVENT_AXIS;
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

    pointer->frame.mask |= WSI_WL_POINTER_EVENT_AXIS_SOURCE;
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

    pointer->frame.mask |= WSI_WL_POINTER_EVENT_AXIS_STOP;
    pointer->frame.axes[axis].stop_time = time;
}

static void
wl_pointer_axis_discrete(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t axis,
    int32_t discrete)
{
    struct wsi_pointer *pointer = data;

    pointer->frame.mask |= WSI_WL_POINTER_EVENT_AXIS_DISCRETE;
    pointer->frame.axes[axis].discrete = discrete * 120;
}

static void
wl_pointer_axis_value120(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t axis,
    int32_t value)
{
    struct wsi_pointer *pointer = data;

    pointer->frame.mask |= WSI_WL_POINTER_EVENT_AXIS_DISCRETE;
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

static enum wsi_result
wsi_pointer_init(struct wsi_seat *seat, struct wsi_event_queue *queue)
{
    assert(seat->pointer == NULL);

    struct wsi_pointer *ptr = calloc(1, sizeof(struct wsi_pointer));
    if (ptr == NULL) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    struct wsi_platform *plat = seat->global.platform;

    ptr->seat = seat;

    ptr->wl_pointer = wl_seat_get_pointer(seat->wl_seat);
    wl_pointer_add_listener(ptr->wl_pointer, &wl_pointer_listener, ptr);

    ptr->wl_cursor_theme = wl_cursor_theme_load(NULL, 24, plat->wl_shm);
    ptr->wl_cursor_surface = wl_compositor_create_surface(plat->wl_compositor);

    seat->pointer = ptr;
    return WSI_SUCCESS;
}

static void
wsi_pointer_uninit(struct wsi_pointer *pointer)
{
    if (wl_pointer_get_version(pointer->wl_pointer) >=
        WL_POINTER_RELEASE_SINCE_VERSION)
    {
        wl_pointer_release(pointer->wl_pointer);
    } else {
        wl_pointer_destroy(pointer->wl_pointer);
    }

    wl_cursor_theme_destroy(pointer->wl_cursor_theme);
    wl_surface_destroy(pointer->wl_cursor_surface);

    memset(pointer, 0, sizeof(struct wsi_pointer));
}

static void
wsi_keyboard_reset(struct wsi_keyboard *keyboard)
{
    xkb_state_unref(keyboard->xkb_state);
    keyboard->xkb_state = NULL;

    xkb_keymap_unref(keyboard->xkb_keymap);
    keyboard->xkb_keymap = NULL;
}

static bool
wsi_keyboard_init_xkb(struct wsi_keyboard *keyboard, int fd, uint32_t size)
{
    char *map_str = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (map_str == MAP_FAILED) {
        return false;
    }

    struct xkb_keymap *keymap = xkb_keymap_new_from_string(
        keyboard->xkb_context,
        map_str,
        XKB_KEYMAP_FORMAT_TEXT_V1,
        XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_str, size);
    if (!keymap) {
        return false;
    }

    struct xkb_state *state = xkb_state_new(keymap);
    if (!state) {
        xkb_keymap_unref(keymap);
        return false;
    }

    keyboard->xkb_keymap = keymap;
    keyboard->xkb_state = state;
    return true;
}

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

    wsi_keyboard_reset(keyboard);

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    wsi_keyboard_init_xkb(keyboard, fd, size);
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

static enum wsi_result
wsi_keyboard_init(struct wsi_seat *seat, struct wsi_event_queue *eq)
{
    assert(seat->keyboard == NULL);

    struct wsi_keyboard *kbd = calloc(1, sizeof(struct wsi_keyboard));
    if (!kbd) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    struct wsi_platform *plat = seat->global.platform;

    kbd->seat = seat;
    kbd->xkb_context = xkb_context_ref(plat->xkb_context);

    kbd->wl_keyboard = wl_seat_get_keyboard(seat->wl_seat);
    wl_keyboard_add_listener(kbd->wl_keyboard, &wl_keyboard_listener, kbd);

    seat->keyboard = kbd;
    return WSI_SUCCESS;
}

static void
wsi_keyboard_uninit(struct wsi_keyboard *keyboard)
{
    if (wl_keyboard_get_version(keyboard->wl_keyboard) >=
        WL_KEYBOARD_RELEASE_SINCE_VERSION)
    {
        wl_keyboard_release(keyboard->wl_keyboard);
    } else {
        wl_keyboard_destroy(keyboard->wl_keyboard);
    }

    xkb_state_unref(keyboard->xkb_state);
    xkb_keymap_unref(keyboard->xkb_keymap);
    xkb_context_unref(keyboard->xkb_context);

    memset(keyboard, 0, sizeof(struct wsi_keyboard));
}

// region WL Seat

static void
wl_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities)
{
    struct wsi_seat *seat = data;

    seat->capabilities = capabilities;
}

static void
wl_seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{
    struct wsi_seat *seat = data;
}

const static struct wl_seat_listener wl_seat_listener = {
    .capabilities = wl_seat_capabilities,
    .name         = wl_seat_name,
};

// endregion

struct wsi_seat *
wsi_seat_bind(struct wsi_platform *platform, uint32_t name, uint32_t version)
{
    struct wsi_seat *seat = calloc(1, sizeof(struct wsi_seat));
    if (!seat) {
        return NULL;
    }

    seat->global.platform = platform;
    seat->global.name = name;
    seat->id = wsi_new_id(platform);

    seat->wl_seat = wsi_bind(
        platform,
        name,
        &wl_seat_interface,
        version,
        WSI_WL_SEAT_VERSION);
    wl_seat_add_listener(seat->wl_seat, &wl_seat_listener, seat);

    wl_list_insert(&platform->seat_list, &seat->link);
    return seat;
}

void
wsi_seat_destroy(struct wsi_seat *seat)
{
    wl_list_remove(&seat->link);

    if (seat->pointer) {
        wsi_pointer_uninit(seat->pointer);
    }

    if (seat->keyboard) {
        wsi_keyboard_uninit(seat->keyboard);
    }

    free(seat->name);

    if (wl_seat_get_version(seat->wl_seat) >=
        WL_SEAT_RELEASE_SINCE_VERSION)
    {
        wl_seat_release(seat->wl_seat);
    } else {
        wl_seat_destroy(seat->wl_seat);
    }

    free(seat);
}

void
wsi_seat_destroy_all(struct wsi_platform *platform)
{
    struct wsi_seat *seat, *tmp;
    wl_list_for_each_safe(seat, tmp, &platform->seat_list, link) {
        wsi_seat_destroy(seat);
    }
}

WsiResult
wsiEnumerateSeats(WsiPlatform platform, uint32_t *pSeatCount, WsiSeat *pSeats)
{
    uint32_t count = 0;

    struct wsi_seat *seat;
    wl_list_for_each_reverse(seat, &platform->seat_list, link) {
        if (pSeats && count < *pSeatCount) {
            pSeats[count] = seat->id;
        }
        count++;
    }

    if (!pSeats) {
        *pSeatCount = count;
        return WSI_SUCCESS;
    }

    if (count > *pSeatCount) {
        return WSI_INCOMPLETE;
    }

    *pSeatCount = count;
    return WSI_SUCCESS;
}

WsiResult
wsiCreatePointer(
    WsiPlatform platform,
    const WsiPointerCreateInfo *pCreateInfo,
    WsiPointer *pPointer)
{
    struct wsi_seat *seat = wsi_seat_find(platform, pCreateInfo->seat);

    if (!seat) {
        return WSI_ERROR_SEAT_LOST;
    }

    if (seat->pointer) {
        return WSI_ERROR_SEAT_IN_USE;
    }

    if (!(seat->capabilities & WL_SEAT_CAPABILITY_POINTER)) {
        return WSI_ERROR_UNSUPPORTED;
    }

    enum wsi_result res = wsi_pointer_init(seat, pCreateInfo->eventQueue);
    if (res == WSI_SUCCESS) {
        *pPointer = seat->pointer;
    }
    return res;
}

void
wsiDestroyPointer(WsiPointer pointer)
{
    struct wsi_seat *seat = pointer->seat;
    if (seat != NULL) {
        assert(seat->pointer == pointer);
        wsi_pointer_uninit(pointer);
        seat->pointer = NULL;
    }
    free(pointer);
}

WsiResult
wsiCreateKeyboard(
    WsiPlatform platform,
    const WsiKeyboardCreateInfo *pCreateInfo,
    WsiKeyboard *pKeyboard)
{
    struct wsi_seat *seat = wsi_seat_find(platform, pCreateInfo->seat);

    if (!seat) {
        return WSI_ERROR_SEAT_LOST;
    }

    if (seat->keyboard) {
        return WSI_ERROR_SEAT_IN_USE;
    }

    if (!(seat->capabilities & WL_SEAT_CAPABILITY_KEYBOARD)) {
        return WSI_ERROR_UNSUPPORTED;
    }

    enum wsi_result res = wsi_keyboard_init(seat, pCreateInfo->eventQueue);
    if (res == WSI_SUCCESS) {
        *pKeyboard = seat->keyboard;
    }
    return res;
}

void
wsiDestroyKeyboard(WsiKeyboard keyboard)
{
    struct wsi_seat *seat = keyboard->seat;
    if (seat != NULL) {
        assert(seat->keyboard == keyboard);
        wsi_keyboard_uninit(keyboard);
        seat->keyboard = NULL;
    }
    free(keyboard);
}
