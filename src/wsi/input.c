#include <dlfcn.h>

#include "wsi/input.h"

extern void *g_handle;

WsiResult
wsiEnumerateSeats(WsiPlatform platform, uint32_t *pIdCount, uint64_t *pIds)
{
    PFN_wsiEnumerateSeats sym
        = (PFN_wsiEnumerateSeats)dlsym(g_handle, "wsiEnumerateSeats");
    return sym(platform, pIdCount, pIds);
}

WsiResult
wsiAcquireSeat(WsiPlatform platform, const WsiAcquireSeatInfo *pAcquireInfo, WsiSeat *pSeat)
{
    PFN_wsiAcquireSeat sym
        = (PFN_wsiAcquireSeat)dlsym(g_handle, "wsiAcquireSeat");
    return sym(platform, pAcquireInfo, pSeat);
}

void
wsiReleaseSeat(WsiSeat seat)
{
    PFN_wsiReleaseSeat sym
        = (PFN_wsiReleaseSeat)dlsym(g_handle, "wsiReleaseSeat");
    return sym(seat);
}
