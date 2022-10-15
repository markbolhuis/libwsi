#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>

#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "platform_priv.h"
#include "seat_priv.h"
#include "keyboard_priv.h"

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

bool
wsi_keyboard_create(struct wsi_seat *seat)
{
    assert(seat->keyboard == NULL);

    struct wsi_keyboard *keyboard = calloc(1, sizeof(struct wsi_keyboard));
    if (!keyboard) {
        return false;
    }

    struct wsi_platform *platform = seat->global.platform;

    keyboard->seat = seat;
    keyboard->xkb_context = xkb_context_ref(platform->xkb_context);

    keyboard->wl_keyboard = wl_seat_get_keyboard(seat->wl_seat);
    wl_keyboard_add_listener(keyboard->wl_keyboard, &wl_keyboard_listener, keyboard);

    seat->keyboard = keyboard;

    return true;
}

void
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
