#ifndef WSI_INCLUDE_INPUT_H
#define WSI_INCLUDE_INPUT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef WsiResult (*PFN_wsiEnumerateSeats)(WsiPlatform platform, uint32_t *pSeatCount, WsiSeat *pSeats);

WsiResult
wsiEnumerateSeats(WsiPlatform platform, uint32_t *pSeatCount, WsiSeat *pSeats);

#ifdef __cplusplus
}
#endif

#endif
