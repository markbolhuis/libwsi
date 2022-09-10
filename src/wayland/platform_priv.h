#ifndef WSI_SRC_WAYLAND_PLATFORM_PRIVATE_H
#define WSI_SRC_WAYLAND_PLATFORM_PRIVATE_H

struct wsi_platform {
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;

    struct wl_list seat_list;
    struct wl_list output_list;

    struct wl_compositor *wl_compositor;
    struct xdg_wm_base *xdg_wm_base;
    struct zxdg_decoration_manager_v1 *xdg_decoration_manager_v1;
    struct zxdg_output_manager_v1 *xdg_output_manager_v1;
};

void *
wsi_platform_bind(
    struct wsi_platform *platform,
    uint32_t name,
    const struct wl_interface *wl_interface,
    uint32_t version);

#endif
