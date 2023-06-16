#ifndef WSI_SRC_WAYLAND_PLATFORM_PRIVATE_H
#define WSI_SRC_WAYLAND_PLATFORM_PRIVATE_H

#include "wsi/platform.h"

struct wsi_global {
    struct wsi_platform *platform;
    uint64_t id;
    uint32_t name;
    uint32_t version;
};

struct wsi_platform {
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;

    struct wl_list seat_list;
    struct wl_list output_list;
    struct wl_list window_list;

    struct wl_array format_array;

    uint64_t id;

    struct wl_compositor *wl_compositor;
    struct wl_shm *wl_shm;
    struct wp_viewporter *wp_viewporter;
    struct wp_fractional_scale_manager_v1 *wp_fractional_scale_manager_v1;
    struct xdg_wm_base *xdg_wm_base;
    struct zxdg_decoration_manager_v1 *xdg_decoration_manager_v1;
    struct zxdg_output_manager_v1 *xdg_output_manager_v1;

    struct xkb_context *xkb_context;
};

uint64_t
wsi_new_id(struct wsi_platform *platform);

int
wsi_flush(struct wl_display *display);

#endif
