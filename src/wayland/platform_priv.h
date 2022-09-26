#ifndef WSI_SRC_WAYLAND_PLATFORM_PRIVATE_H
#define WSI_SRC_WAYLAND_PLATFORM_PRIVATE_H

struct wsi_platform {
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;

    struct wl_list seat_list;
    struct wl_list output_list;

    struct wl_compositor *wl_compositor;
    struct wl_shm *wl_shm;
    struct xdg_wm_base *xdg_wm_base;
    struct zxdg_decoration_manager_v1 *xdg_decoration_manager_v1;
    struct zxdg_output_manager_v1 *xdg_output_manager_v1;
};

struct wsi_global {
    struct wsi_platform *platform;
    uint32_t name;
};

void *
wsi_platform_bind(
    struct wsi_platform *platform,
    uint32_t name,
    const struct wl_interface *wl_interface,
    uint32_t version);

struct wsi_global *
wsi_global_create(
    struct wsi_platform *platform,
    uint32_t name);

void
wsi_global_destroy(
    struct wsi_global *global);

#endif
