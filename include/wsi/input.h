#ifndef WSI_INCLUDE_INPUT_H
#define WSI_INCLUDE_INPUT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t WsiSeatId;

typedef struct wsi_seat_create_info {
    WsiEventQueue eventQueue;
    WsiSeatId id;
} WsiSeatCreateInfo;

typedef WsiResult (*PFN_wsiCreateSeat)(WsiPlatform platform, const WsiSeatCreateInfo *pCreateInfo, WsiSeat *pSeat);
typedef void (*PFN_wsiDestroySeat)(WsiSeat seat);
typedef WsiResult (*PFN_wsiEnumerateSeats)(WsiPlatform platform, uint32_t *pSeatCount, WsiSeatId *pSeats);

WsiResult
wsiCreateSeat(WsiPlatform platform, const WsiSeatCreateInfo *pCreateInfo, WsiSeat *pSeat);

void
wsiDestroySeat(WsiSeat seat);

WsiResult
wsiEnumerateSeats(WsiPlatform platform, uint32_t *pSeatCount, WsiSeatId *pSeats);

#ifdef __cplusplus
}
#endif

#endif
