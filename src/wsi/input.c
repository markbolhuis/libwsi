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

WsiResult
wsiCreatePointer(WsiPlatform platform, const WsiPointerCreateInfo *pCreateInfo, WsiPointer *pPointer)
{
    struct wsi_pointer *pointer = calloc(1, sizeof(struct wsi_pointer));
    if (!pointer) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    PFN_wsiCreatePointer sym = wsi_platform_dlsym(platform, "wsiCreatePointer");
    enum wsi_result result = sym(platform->platform, pCreateInfo, &pointer->pointer);
    if (result != WSI_SUCCESS) {
        free(pointer);
        return result;
    }

    pointer->platform = platform;
    *pPointer = pointer;
    return WSI_SUCCESS;
}

void
wsiDestroyPointer(WsiPointer pointer)
{
    PFN_wsiDestroyPointer sym = wsi_platform_dlsym(pointer->platform, "wsiDestroyPointer");
    sym(pointer->pointer);
    free(pointer);
}

WsiResult
wsiCreateKeyboard(WsiPlatform platform, const WsiKeyboardCreateInfo *pCreateInfo, WsiKeyboard *pKeyboard)
{
    struct wsi_keyboard *keyboard = calloc(1, sizeof(struct wsi_keyboard));
    if (!keyboard) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    PFN_wsiCreateKeyboard sym = wsi_platform_dlsym(platform, "wsiCreateKeyboard");
    enum wsi_result result = sym(platform->platform, pCreateInfo, &keyboard->keyboard);
    if (result != WSI_SUCCESS) {
        free(keyboard);
        return result;
    }

    keyboard->platform = platform;
    *pKeyboard = keyboard;
    return WSI_SUCCESS;
}

void
wsiDestroyKeyboard(WsiKeyboard keyboard)
{
    PFN_wsiDestroyKeyboard sym = wsi_platform_dlsym(keyboard->platform, "wsiDestroyKeyboard");
    sym(keyboard->keyboard);
    free(keyboard);
}
