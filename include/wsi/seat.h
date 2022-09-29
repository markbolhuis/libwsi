#ifndef WSI_INCLUDE_SEAT_H
#define WSI_INCLUDE_SEAT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t WsiNativeSeat;

typedef struct wsi_seat_create_info {
    WsiEventQueue eventQueue;
    WsiNativeSeat nativeSeat;
} WsiSeatCreateInfo;

WsiResult
wsiCreateSeat(WsiPlatform platform, const WsiSeatCreateInfo *pCreateInfo, WsiSeat *pSeat);

void
wsiDestroySeat(WsiSeat seat);

WsiResult
wsiEnumerateNativeSeats(WsiPlatform platform, uint32_t *pNativeSeatCount, WsiNativeSeat *pNativeSeats);

#ifdef __cplusplus
}
#endif

#endif
