#ifndef WSI_SRC_WAYLAND_WINDOW_PRIVATE_H
#define WSI_SRC_WAYLAND_WINDOW_PRIVATE_H

enum wsi_xdg_event {
    WSI_XDG_EVENT_NONE = 0,
    WSI_XDG_EVENT_CONFIGURE = 1,
    WSI_XDG_EVENT_BOUNDS = 2,
    WSI_XDG_EVENT_WM_CAPABILITIES = 4,
    WSI_XDG_EVENT_DECORATION = 8,
};

enum wsi_xdg_capabilities {
    WSI_XDG_CAPABILITIES_NONE = 0,
    WSI_XDG_CAPABILITY_WINDOW_MENU = 1,
    WSI_XDG_CAPABILITY_MAXIMIZE = 2,
    WSI_XDG_CAPABILITY_FULLSCREEN = 4,
    WSI_XDG_CAPABILITY_MINIMIZE = 8,
};

enum wsi_xdg_state {
    WSI_XDG_STATE_NONE = 0,
    WSI_XDG_STATE_MAXIMIZED = 1,
    WSI_XDG_STATE_RESIZING = 2,
    WSI_XDG_STATE_FULLSCREEN = 4,
    WSI_XDG_STATE_ACTIVATED = 8,
    WSI_XDG_STATE_TILED_TOP = 16,
    WSI_XDG_STATE_TILED_BOTTOM = 32,
    WSI_XDG_STATE_TILED_LEFT = 64,
    WSI_XDG_STATE_TILED_RIGHT = 128,
};

struct wsi_window_state {
    struct wsi_wl_extent extent;            // xdg_toplevel.configure
    struct wsi_wl_extent bounds;            // xdg_toplevel.configure_bounds
    enum wsi_xdg_state state;               // xdg_toplevel.configure
    enum wsi_xdg_capabilities capabilities; // xdg_toplevel.wm_capabilities
    int32_t scale;                          // wl_surface.{enter.leave}
    uint32_t decoration;                    // xdg_toplevel_decoration.configure
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

    PFN_wsiCloseWindow pfn_close;
    PFN_wsiResizeWindow pfn_resize;

    void *user_data;
};

void
wsi_window_handle_output_destroyed(
    struct wsi_window *window,
    struct wsi_output *output);

struct wsi_wl_extent
wsi_window_get_buffer_extent(struct wsi_window *window);

#endif
