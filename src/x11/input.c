#include <stdio.h>
#include <stdlib.h>

#include <xcb/xcb.h>
#include <xcb/xinput.h>

#include "wsi/input.h"

#include "utils.h"

#include "platform_priv.h"
#include "input_priv.h"

WsiResult
wsiEnumerateSeats(WsiPlatform platform, uint32_t *pSeatCount, WsiSeat *pSeats)
{
    // TODO: Implement
    abort();
}

WsiResult
wsiCreatePointer(
    WsiPlatform platform,
    const WsiPointerCreateInfo *pCreateInfo,
    WsiPointer *pPointer)
{
    // TODO: Implement
    abort();
}

void
wsiDestroyPointer(WsiPointer pointer)
{
    // TODO: Implement
    abort();
}

WsiResult
wsiCreateKeyboard(
    WsiPlatform platform,
    const WsiKeyboardCreateInfo *pCreateInfo,
    WsiKeyboard *pKeyboard)
{
    // TODO: Implement
    abort();
}

void
wsiDestroyKeyboard(WsiKeyboard keyboard)
{
    // TODO: Implement
}
