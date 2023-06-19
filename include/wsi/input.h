#ifndef WSI_INCLUDE_INPUT_H
#define WSI_INCLUDE_INPUT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WsiAcquireSeatInfo {
    WsiStructureType sType;
    const void *pNext;
    uint64_t id;
} WsiAcquireSeatInfo;

typedef WsiResult (*PFN_wsiEnumerateSeats)(WsiPlatform platform, uint32_t *pIdCount, uint64_t *pIds);
typedef WsiResult (*PFN_wsiAcquireSeat)(WsiPlatform platform, const WsiAcquireSeatInfo *pCreateInfo, WsiSeat *pSeat);
typedef void (*PFN_wsiReleaseSeat)(WsiSeat seat);

WsiResult
wsiEnumerateSeats(WsiPlatform platform, uint32_t *pIdCount, uint64_t *pIds);

WsiResult
wsiAcquireSeat(WsiPlatform platform, const WsiAcquireSeatInfo *pAcquireInfo, WsiSeat *pSeat);

void
wsiReleaseSeat(WsiSeat seat);

#ifdef __cplusplus
}
#endif

#endif
