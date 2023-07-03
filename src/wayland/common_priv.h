#ifndef WSI_SRC_WAYLAND_COMMON_PRIVATE_H
#define WSI_SRC_WAYLAND_COMMON_PRIVATE_H

#include "wsi/common.h"

static inline bool
wsi_extent_equal(WsiExtent a, WsiExtent b)
{
    return a.width == b.width && a.height == b.height;
}

static inline int32_t
wsi_div_round(int32_t a, int32_t b)
{
    return (a + b / 2) / b;
}

static inline int64_t
wsi_tv_to_ns(uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec)
{
    int64_t tv_sec = ((int64_t)tv_sec_hi << 32) | (int64_t)tv_sec_lo;
    return (tv_sec * 1000000000) + (int64_t)tv_nsec;
}

static inline int64_t
wsi_us_to_ns(uint32_t utime_hi, uint32_t utime_lo)
{
    return (((int64_t)utime_hi) << 32 | (int64_t)utime_lo) * 1000;
}

static inline int64_t
wsi_ms_to_ns(uint32_t mtime)
{
    return ((int64_t)mtime) * 1000000;
}

#endif
