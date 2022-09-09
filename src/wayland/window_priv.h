#ifndef WSI_SRC_WAYLAND_WINDOW_PRIVATE_H
#define WSI_SRC_WAYLAND_WINDOW_PRIVATE_H

struct wsi_extent {
    int32_t width;
    int32_t height;
};

struct wsi_window {
    struct wsi_platform *platform;
    struct wsi_window   *parent;
    struct wsi_extent   extent;
    struct wsi_extent   user_extent;

    struct wl_surface   *wl_surface;
    struct xdg_surface  *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct zxdg_toplevel_decoration_v1 *xdg_toplevel_decoration_v1;

    int32_t scale;
    struct wl_list output_list;
};

struct wsi_vk_window {
    struct wsi_window base;
};

struct wsi_egl_window {
    struct wsi_window base;
    struct wl_egl_window *wl_egl_window;
};

#endif
