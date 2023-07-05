#ifndef WSI_SRC_XCB_COMMON_PRIVATE_H
#define WSI_SRC_XCB_COMMON_PRIVATE_H

#include "wsi/common.h"

static inline uint16_t
wsi_xcb_clamp(int32_t value)
{
    if (value < 0) {
        return 0;
    }
    if (value > UINT16_MAX) {
        return UINT16_MAX;
    }
    return (uint16_t)value;
}

enum wsi_api {
    WSI_API_NONE = 0,
    WSI_API_EGL = 1,
    WSI_API_VULKAN = 2,
};

#endif