#ifndef WSI_SRC_WAYLAND_COMMON_PRIVATE_H
#define WSI_SRC_WAYLAND_COMMON_PRIVATE_H

static inline bool
wsi_extent_equal(struct wsi_extent a, struct wsi_extent b)
{
    return a.width == b.width && a.height == b.height;
}

#endif
