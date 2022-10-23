#ifndef WSI_INCLUDE_INPUT_H
#define WSI_INCLUDE_INPUT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wsi_pointer_create_info {
    WsiEventQueue eventQueue;
    WsiSeat seat;
} WsiPointerCreateInfo;

typedef struct wsi_keyboard_create_info {
    WsiEventQueue eventQueue;
    WsiSeat seat;
} WsiKeyboardCreateInfo;

typedef WsiResult (*PFN_wsiEnumerateSeats)(WsiPlatform platform, uint32_t *pSeatCount, WsiSeat *pSeats);
typedef WsiResult (*PFN_wsiCreatePointer)(WsiPlatform platform, const WsiPointerCreateInfo *pCreateInfo, WsiPointer *pPointer);
typedef void (*PFN_wsiDestroyPointer)(WsiPointer pointer);
typedef WsiResult (*PFN_wsiCreateKeyboard)(WsiPlatform platform, const WsiKeyboardCreateInfo *pCreateInfo, WsiKeyboard *pKeyboard);
typedef void (*PFN_wsiDestroyKeyboard)(WsiKeyboard keyboard);

WsiResult
wsiEnumerateSeats(WsiPlatform platform, uint32_t *pSeatCount, WsiSeat *pSeats);

WsiResult
wsiCreatePointer(WsiPlatform platform, const WsiPointerCreateInfo *pCreateInfo, WsiPointer *pPointer);

void
wsiDestroyPointer(WsiPointer pointer);

WsiResult
wsiCreateKeyboard(WsiPlatform platform, const WsiKeyboardCreateInfo *pCreateInfo, WsiKeyboard *pKeyboard);

void
wsiDestroyKeyboard(WsiKeyboard keyboard);

#ifdef __cplusplus
}
#endif

#endif
