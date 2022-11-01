#include <dlfcn.h>

#include "wsi/input.h"

extern void *g_handle;

WsiResult
wsiEnumerateSeats(WsiPlatform platform, uint32_t *pSeatCount, WsiSeat *pSeats)
{
    PFN_wsiEnumerateSeats sym
        = (PFN_wsiEnumerateSeats)dlsym(g_handle, "wsiEnumerateSeats");
    return sym(platform, pSeatCount, pSeats);
}

WsiResult
wsiCreatePointer(WsiPlatform platform, const WsiPointerCreateInfo *pCreateInfo, WsiPointer *pPointer)
{
    PFN_wsiCreatePointer sym
        = (PFN_wsiCreatePointer)dlsym(g_handle, "wsiCreatePointer");
    return sym(platform, pCreateInfo, pPointer);
}

void
wsiDestroyPointer(WsiPointer pointer)
{
    PFN_wsiDestroyPointer sym
        = (PFN_wsiDestroyPointer)dlsym(g_handle, "wsiDestroyPointer");
    sym(pointer);
}

WsiResult
wsiCreateKeyboard(WsiPlatform platform, const WsiKeyboardCreateInfo *pCreateInfo, WsiKeyboard *pKeyboard)
{
    PFN_wsiCreateKeyboard sym
        = (PFN_wsiCreateKeyboard)dlsym(g_handle, "wsiCreateKeyboard");
    return sym(platform, pCreateInfo, pKeyboard);
}

void
wsiDestroyKeyboard(WsiKeyboard keyboard)
{
    PFN_wsiDestroyKeyboard sym
        = (PFN_wsiDestroyKeyboard)dlsym(g_handle, "wsiDestroyKeyboard");
    sym(keyboard);
}
