#ifndef WSI_SRC_COCOA_COMMON_PRIVATE_H
#define WSI_SRC_COCOA_COMMON_PRIVATE_H

#include "wsi/common.h"

static inline uint16_t
wsi_cocoa_clamp(int32_t value)
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
    WSI_API_VULKAN = 1,
};

#endif