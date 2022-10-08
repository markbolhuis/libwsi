#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#include <wayland-client-protocol.h>

#include <xkbcommon/xkbcommon.h>

#include "wsi/seat.h"

#include "platform_priv.h"
#include "seat_priv.h"

#define WSI_WL_SEAT_VERSION 7

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
wsi_keyboard_create(struct wsi_seat *seat)
{
    assert(seat->keyboard == NULL);

    struct wsi_keyboard *keyboard = calloc(1, sizeof(struct wsi_keyboard));
    if (!keyboard) {
        return false;
    }

    struct wsi_platform *platform = seat->global.platform;

    keyboard->xkb_context = xkb_context_ref(platform->xkb_context);

    keyboard->wl_keyboard = wl_seat_get_keyboard(seat->wl_seat);
    wl_keyboard_add_listener(keyboard->wl_keyboard, &wl_keyboard_listener, keyboard);

    seat->keyboard = keyboard;

    return true;
}

static void
wsi_keyboard_destroy(struct wsi_seat *seat)
{
    assert(seat->keyboard != NULL);

    struct wsi_keyboard *keyboard = seat->keyboard;
    seat->keyboard = NULL;

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

    free(keyboard);
}

// region WL Seat

static void
wl_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities)
{
    struct wsi_seat *seat = data;

    seat->capabilities = capabilities;

    bool has_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

    if (has_keyboard && !seat->keyboard) {
        wsi_keyboard_create(seat);
    } else if (!has_keyboard && seat->keyboard) {
        wsi_keyboard_destroy(seat);
    }
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

    seat->wl_seat = wsi_bind(
        platform,
        name,
        &wl_seat_interface,
        version,
        WSI_WL_SEAT_VERSION);
    wl_seat_add_listener(seat->wl_seat, &wl_seat_listener, seat);

    wl_list_insert(&platform->seat_list, &seat->link);

    seat->id = wsi_platform_new_id(platform);
    seat->ref_count = 1;

    return seat;
}

void
wsi_seat_destroy(struct wsi_seat *seat)
{
    assert(seat);
    assert(seat->ref_count > 0);
    assert(seat->wl_seat != NULL);

    wl_list_remove(&seat->link);

    if (seat->keyboard) {
        wsi_keyboard_destroy(seat);
    }

    if (wl_seat_get_version(seat->wl_seat) >=
        WL_SEAT_RELEASE_SINCE_VERSION)
    {
        wl_seat_release(seat->wl_seat);
    } else {
        wl_seat_destroy(seat->wl_seat);
    }

    seat->wl_seat = NULL;
    seat->capabilities = 0;
    seat->global.name = 0;

    if (seat->name) {
        free(seat->name);
        seat->name = NULL;
    }

    if (--seat->ref_count == 0) {
        free(seat);
    }
}

void
wsi_seat_destroy_all(struct wsi_platform *platform)
{
    struct wsi_seat *seat, *seat_tmp;
    wl_list_for_each_safe(seat, seat_tmp, &platform->seat_list, link) {
        wsi_seat_destroy(seat);
    }
}

WsiResult
wsiCreateSeat(WsiPlatform platform, const WsiSeatCreateInfo *pCreateInfo, WsiSeat *pSeat)
{
    WsiNativeSeat native_seat = pCreateInfo->nativeSeat;

    if (native_seat == 0) {
        return WSI_ERROR_UNKNOWN;
    }

    bool found = false;

    struct wsi_seat *seat;
    wl_list_for_each(seat, &platform->seat_list, link) {
        if (seat->id == native_seat) {
            found = true;
            break;
        }
    }

    if (!found) {
        return WSI_ERROR_NATIVE_SEAT_LOST;
    }

    assert(seat->wl_seat != NULL);
    assert(seat->ref_count == 1);

    seat->ref_count++;
    *pSeat = seat;

    return WSI_SUCCESS;
}

void
wsiDestroySeat(WsiSeat seat)
{
    assert(seat->ref_count > 0);
    if (--seat->ref_count == 0) {
        assert(seat->wl_seat == NULL);
        free(seat);
    }
}

WsiResult
wsiEnumerateNativeSeats(
    WsiPlatform platform,
    uint32_t *pNativeSeatCount,
    WsiNativeSeat *pNativeSeats)
{
    uint32_t count = 0;

    struct wsi_seat *seat;
    wl_list_for_each_reverse(seat, &platform->seat_list, link) {
        if (pNativeSeats && count < *pNativeSeatCount) {
            pNativeSeats[count] = seat->id;
        }
        count++;
    }

    if (!pNativeSeats) {
        *pNativeSeatCount = count;
        return WSI_SUCCESS;
    }

    if (count > *pNativeSeatCount) {
        return WSI_INCOMPLETE;
    }

    *pNativeSeatCount = count;
    return WSI_SUCCESS;
}