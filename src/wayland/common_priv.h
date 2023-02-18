#ifndef WSI_SRC_WAYLAND_COMMON_PRIVATE_H
#define WSI_SRC_WAYLAND_COMMON_PRIVATE_H

#include "wsi/common.h"

static inline bool
wsi_extent_equal(WsiExtent a, WsiExtent b)
{
    return a.width == b.width && a.height == b.height;
}

static inline int32_t
div_round(int32_t a, int32_t b)
{
    return (a + b / 2) / b;
}

enum wsi_api {
    WSI_API_NONE = 0,
    WSI_API_EGL = 1,
    WSI_API_VULKAN = 2,
};

#endif
