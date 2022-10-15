#ifndef WSI_SRC_WAYLAND_COMMON_PRIVATE_H
#define WSI_SRC_WAYLAND_COMMON_PRIVATE_H

struct wsi_wl_extent {
    int32_t width;
    int32_t height;
};

static inline struct wsi_wl_extent
wsi_extent_to_wl(
    struct wsi_extent extent)
{
    assert(extent.width <= INT32_MAX);
    assert(extent.height <= INT32_MAX);

    struct wsi_wl_extent wl = {
        .width = (int32_t)extent.width,
        .height = (int32_t)extent.height,
    };

    return wl;
}

static inline struct wsi_extent
wsi_extent_from_wl(
    struct wsi_wl_extent wl)
{
    assert(wl.width > 0);
    assert(wl.height > 0);

    struct wsi_extent extent = {
        .width = (uint32_t)wl.width,
        .height = (uint32_t)wl.height,
    };

    return extent;
}

static inline bool
wsi_wl_extent_equal(struct wsi_wl_extent a, struct wsi_wl_extent b)
{
    return a.width == b.width && a.height == b.height;
}

#endif
