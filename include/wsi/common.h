#ifndef WSI_INCLUDE_COMMON_H
#define WSI_INCLUDE_COMMON_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wsi_platform *WsiPlatform;
typedef struct wsi_event_queue *WsiEventQueue;
typedef struct wsi_seat *WsiSeat;
typedef struct wsi_window *WsiWindow;
typedef struct wsi_output *WsiOutput;
typedef struct wsi_pointer *WsiPointer;
typedef struct wsi_keyboard *WsiKeyboard;

typedef enum wsi_result {
    WSI_SUCCESS = 0,
    WSI_INCOMPLETE = 1,
    WSI_TIMEOUT = 2,
    WSI_EVENT_SET = 3,
    WSI_EVENT_UNSET = 4,
    WSI_SKIPPED = 5,
    WSI_ERROR_UNKNOWN = -1,
    WSI_ERROR_EGL = -2,
    WSI_ERROR_VULKAN = -3,
    WSI_ERROR_OUT_OF_RANGE = -4,
    WSI_ERROR_OUT_OF_MEMORY = -5,
    WSI_ERROR_UNSUPPORTED = -6,
    WSI_ERROR_DISCONNECTED = -7,
    WSI_ERROR_PLATFORM = -8,
    WSI_ERROR_PLATFORM_LOST = -9,
    WSI_ERROR_NATIVE_SEAT_LOST = -10,
    WSI_ERROR_SEAT_DISCONNECTED = -11,
    WSI_ERROR_NOT_IMPLEMENTED = -12,
    WSI_ERROR_UNINITIALIZED = -13,
    WSI_ERROR_WINDOW_IN_USE = -14,
    WSI_ERROR_ENUM_MAX = 0x7fffffff,
} WsiResult;

typedef struct wsi_extent {
    uint32_t width;
    uint32_t height;
} WsiExtent;

typedef enum wsi_api {
    WSI_API_NONE = 0,
    WSI_API_EGL = 1,
    WSI_API_VULKAN = 2,
    WSI_API_ENUM_MAX = 0x7fffffff,
} WsiApi;

#ifdef __cplusplus
}
#endif

#endif
