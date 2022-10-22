#include <stdlib.h>

#include "wsi/input.h"

#include "platform_priv.h"
#include "input_priv.h"

WsiResult
wsiEnumerateSeats(WsiPlatform platform, uint32_t *pSeatCount, WsiSeat *pSeats)
{
    PFN_wsiEnumerateSeats sym = wsi_platform_dlsym(platform, "wsiEnumerateSeats");
    return sym(platform->platform, pSeatCount, pSeats);
}


