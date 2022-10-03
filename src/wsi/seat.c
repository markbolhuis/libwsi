#include <stdlib.h>

#include "wsi/seat.h"

#include "platform_priv.h"
#include "seat_priv.h"

void *
wsi_seat_dlsym(WsiSeat seat, const char *symbol)
{
    return wsi_platform_dlsym(seat->platform, symbol);
}

WsiResult
wsiCreateSeat(WsiPlatform platform, const WsiSeatCreateInfo *pCreateInfo, WsiSeat *pSeat)
{
    struct wsi_seat *seat = calloc(1, sizeof(struct wsi_seat));
    if (!seat) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    seat->platform = platform;

    PFN_wsiCreateSeat sym = wsi_platform_dlsym(platform, "wsiCreateSeat");
    enum wsi_result result = sym(platform->platform, pCreateInfo, &seat->seat);
    if (result != WSI_SUCCESS) {
        free(seat);
        return result;
    }

    *pSeat = seat;
    return WSI_SUCCESS;
}

void
wsiDestroySeat(WsiSeat seat)
{
    PFN_wsiDestroySeat sym = wsi_seat_dlsym(seat, "wsiDestroySeat");
    sym(seat->seat);
    free(seat);
}

WsiResult
wsiEnumerateNativeSeats(WsiPlatform platform, uint32_t *pSeatCount, WsiNativeSeat *pSeats)
{
    PFN_wsiEnumerateNativeSeats sym = wsi_platform_dlsym(platform, "wsiEnumerateNativeSeats");
    return sym(platform->platform, pSeatCount, pSeats);
}


