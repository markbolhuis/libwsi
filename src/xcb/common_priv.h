#ifndef WSI_SRC_XCB_COMMON_PRIVATE_H
#define WSI_SRC_XCB_COMMON_PRIVATE_H

struct wsi_xcb_extent {
    uint16_t width;
    uint16_t height;
};

static inline struct wsi_xcb_extent
wsi_extent_to_xcb(
    struct wsi_extent extent)
{
    assert(extent.width <= UINT16_MAX);
    assert(extent.height <= UINT16_MAX);

    struct wsi_xcb_extent xcb = {
        .width = (uint16_t)extent.width,
        .height = (uint16_t)extent.height,
    };

    return xcb;
}

static inline struct wsi_extent
wsi_extent_from_xcb(
    struct wsi_xcb_extent extent)
{
    struct wsi_extent xcb = {
        .width = extent.width,
        .height = extent.height,
    };

    return xcb;
}

#endif