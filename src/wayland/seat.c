#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <wayland-client-protocol.h>

#include <xkbcommon/xkbcommon.h>

#include "wsi/seat.h"

#include "event_queue_priv.h"
#include "platform_priv.h"
#include "seat_priv.h"
#include "pointer_priv.h"
#include "keyboard_priv.h"

#define WSI_WL_SEAT_VERSION 7

// region WL Seat

static void
wl_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities)
{
    struct wsi_seat *seat = data;

    seat->capabilities = capabilities;

    bool has_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
    bool has_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

    if (has_pointer && !seat->pointer) {
        wsi_pointer_create(seat);
    } else if (!has_pointer && seat->pointer) {
        wsi_pointer_destroy(seat);
    }

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

static void
wsi_seat_uninit(struct wsi_seat *seat)
{
    seat->ref->seat = NULL;
    seat->ref = NULL;

    if (seat->pointer) {
        wsi_pointer_destroy(seat);
        seat->pointer = NULL;
    }

    if (seat->keyboard) {
        wsi_keyboard_destroy(seat);
        seat->keyboard = NULL;
    }

    if (seat->name) {
        free(seat->name);
        seat->name = NULL;
    }

    if (seat->wl_seat) {
        if (wl_seat_get_version(seat->wl_seat) >=
            WL_SEAT_RELEASE_SINCE_VERSION)
        {
            wl_seat_release(seat->wl_seat);
        } else {
            wl_seat_destroy(seat->wl_seat);
        }
        seat->wl_seat = NULL;
    }

    seat->global.name = 0;
    seat->capabilities = 0;
}

bool
wsi_seat_ref_add(struct wsi_platform *platform, uint32_t name, uint32_t version)
{
    struct wsi_seat_ref *seat_ref = calloc(1, sizeof(struct wsi_seat_ref));
    if (!seat_ref) {
        return false;
    }

    seat_ref->id = wsi_platform_new_id(platform);
    seat_ref->name = name;
    seat_ref->version = version;

    wl_list_insert(&platform->seat_list, &seat_ref->link);
    return true;
}

void
wsi_seat_ref_remove(struct wsi_seat_ref *seat_ref)
{
    wl_list_remove(&seat_ref->link);

    if (seat_ref->seat) {
        wsi_seat_uninit(seat_ref->seat);
    }

    free(seat_ref);
}

void
wsi_seat_ref_remove_all(struct wsi_platform *platform)
{
    struct wsi_seat_ref *ref, *tmp;
    wl_list_for_each_safe(ref, tmp, &platform->seat_list, link) {
        wsi_seat_ref_remove(ref);
    }
}

WsiResult
wsiCreateSeat(WsiPlatform platform, const WsiSeatCreateInfo *pCreateInfo, WsiSeat *pSeat)
{
    bool found = false;

    struct wsi_seat_ref *ref;
    wl_list_for_each(ref, &platform->seat_list, link) {
        if (ref->id == pCreateInfo->id) {
            found = true;
            break;
        }
    }

    if (!found) {
        return WSI_ERROR_SEAT_LOST;
    }

    if (ref->seat) {
        return WSI_ERROR_SEAT_IN_USE;
    }

    struct wsi_seat *seat = calloc(1, sizeof(struct wsi_seat));
    if (!seat) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    seat->global.platform = platform;
    seat->global.name = ref->name;

    seat->wl_seat = wsi_bind(
        platform,
        ref->name,
        &wl_seat_interface,
        ref->version,
        WSI_WL_SEAT_VERSION);
    wl_seat_add_listener(seat->wl_seat, &wl_seat_listener, seat);

    seat->ref = ref;
    ref->seat = seat;
    *pSeat = seat;
    return WSI_SUCCESS;
}

void
wsiDestroySeat(WsiSeat seat)
{
    wsi_seat_uninit(seat);
    free(seat);
}

WsiResult
wsiEnumerateSeats(
    WsiPlatform platform,
    uint32_t *pSeatCount,
    WsiSeatId *pSeats)
{
    uint32_t count = 0;

    struct wsi_seat_ref *seat;
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