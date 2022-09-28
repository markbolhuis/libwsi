#ifndef WSI_SRC_WAYLAND_WINDOW_PRIVATE_H
#define WSI_SRC_WAYLAND_WINDOW_PRIVATE_H

enum wsi_xdg_event {
    WSI_XDG_EVENT_NONE = 0,
    WSI_XDG_EVENT_CONFIGURE = 1,
    WSI_XDG_EVENT_BOUNDS = 2,
    WSI_XDG_EVENT_WM_CAPABILITIES = 4,
    WSI_XDG_EVENT_DECORATION = 8,
};

struct wsi_xdg_capabilities {
    bool window_menu;
    bool maximize;
    bool fullscreen;
    bool minimize;
};

struct wsi_xdg_state {
    bool maximized;
    bool resizing;
    bool fullscreen;
    bool activated;
    bool tiled_top;
    bool tiled_bottom;
    bool tiled_left;
    bool tiled_right;
};

struct wsi_window_state {
    struct wsi_wl_extent extent;              // xdg_toplevel.configure
    struct wsi_wl_extent bounds;              // xdg_toplevel.configure_bounds
    struct wsi_xdg_state state;               // xdg_toplevel.configure
    struct wsi_xdg_capabilities capabilities; // xdg_toplevel.wm_capabilities
    int32_t scale;                            // wl_surface.{enter.leave}
    uint32_t decoration;                      // xdg_toplevel_decoration.configure
};

struct wsi_window {
    struct wsi_platform   *platform;
    struct wsi_window     *parent;
    struct wsi_wl_extent  user_extent;

    struct wl_list link;

    enum wsi_api          api;

    struct wl_surface     *wl_surface;
    struct wl_egl_window  *wl_egl_window; // Temporary
    struct xdg_surface    *xdg_surface;
    struct xdg_toplevel   *xdg_toplevel;
    struct zxdg_toplevel_decoration_v1 *xdg_toplevel_decoration_v1;

    enum wsi_xdg_event event_mask;
    uint32_t serial;
    struct wsi_window_state pending;
    struct wsi_window_state current;
    struct wl_list output_list;

    bool closed;
};

void
wsi_window_handle_output_destroyed(
    struct wsi_window *window,
    struct wsi_output *output);

#endif
