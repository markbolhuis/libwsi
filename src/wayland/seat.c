#include <stdlib.h>
#include <assert.h>

#include <wayland-client-protocol.h>

#include "wsi/seat.h"

#include "platform_priv.h"
#include "seat_priv.h"

// region WL Seat

static void
wl_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities)
{
    struct wsi_seat *seat = data;
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

    seat->platform = platform;
    seat->wl_global_name = name;

    seat->wl_seat = wsi_platform_bind(
        platform,
        name,
        &wl_seat_interface,
        version);
    wl_seat_add_listener(
        seat->wl_seat,
        &wl_seat_listener,
        seat);

    wl_list_insert(&platform->seat_list, &seat->link);
    return seat;
}

void
wsi_seat_destroy(struct wsi_seat *seat)
{
    assert(seat);

    if (seat->wl_seat) {
        if (wl_seat_get_version(seat->wl_seat) >= WL_SEAT_RELEASE_SINCE_VERSION) {
            wl_seat_release(seat->wl_seat);
        } else {
            wl_seat_destroy(seat->wl_seat);
        }
    }

    if (seat->name) {
        free(seat->name);
    }

    wl_list_remove(&seat->link);
    free(seat);
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
        if (seat->wl_global_name == native_seat) {
            found = true;
            break;
        }
    }

    if (!found) {
        return WSI_ERROR_NATIVE_SEAT_LOST;
    }

    *pSeat = seat;
    return WSI_SUCCESS;
}

void
wsiDestroySeat(WsiPlatform platform, WsiSeat seat)
{
    wsi_seat_destroy(seat);
}
