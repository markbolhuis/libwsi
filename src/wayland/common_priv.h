#ifndef WSI_SRC_WAYLAND_COMMON_PRIVATE_H
#define WSI_SRC_WAYLAND_COMMON_PRIVATE_H

#include "wsi/common.h"

static inline bool
wsi_extent_equal(WsiExtent a, WsiExtent b)
{
    return a.width == b.width && a.height == b.height;
}

enum wsi_api {
    WSI_API_NONE = 0,
    WSI_API_EGL = 1,
    WSI_API_VULKAN = 2,
};

#endif
