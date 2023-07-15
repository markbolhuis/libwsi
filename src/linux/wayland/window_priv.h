#ifndef WSI_SRC_WAYLAND_WINDOW_PRIVATE_H
#define WSI_SRC_WAYLAND_WINDOW_PRIVATE_H

#include "wsi/window.h"

struct wsi_output;

enum wsi_xdg_event {
    WSI_XDG_EVENT_NONE = 0,
    WSI_XDG_EVENT_SCALE = 1,
    WSI_XDG_EVENT_TRANSFORM = 2,
    WSI_XDG_EVENT_EXTENT = 4,
    WSI_XDG_EVENT_STATE = 8,
    WSI_XDG_EVENT_BOUNDS = 16,
    WSI_XDG_EVENT_CAPABILITIES = 32,
    WSI_XDG_EVENT_DECORATION = 64,
};

enum wsi_xdg_capabilities {
    WSI_XDG_CAPABILITIES_NONE = 0,
    WSI_XDG_CAPABILITIES_WINDOW_MENU = 1,
    WSI_XDG_CAPABILITIES_MAXIMIZE = 2,
    WSI_XDG_CAPABILITIES_FULLSCREEN = 4,
    WSI_XDG_CAPABILITIES_MINIMIZE = 8,
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
    WSI_XDG_STATE_SUSPENDED = 256,
};

struct wsi_window_state {
    WsiExtent extent;
    WsiExtent bounds;
    enum wsi_xdg_state state;
    enum wsi_xdg_capabilities capabilities;
    int32_t scale;
    int32_t transform;
    uint32_t decoration;
};

struct wsi_window {
    struct wsi_platform *platform;

    struct wl_list link;

    struct wl_surface *wl_surface;
    struct wl_egl_window *wl_egl_window;
    struct wp_viewport *wp_viewport;
    struct wp_fractional_scale_v1 *wp_fractional_scale_v1;
    struct wp_content_type_v1 *wp_content_type_v1;
    struct zwp_idle_inhibitor_v1 *wp_idle_inhibitor_v1;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct zxdg_toplevel_decoration_v1 *xdg_toplevel_decoration_v1;

    bool configured;

    struct wl_list output_list;

    enum wsi_xdg_event event_mask;
    struct wsi_window_state pending;
    struct wsi_window_state current;

    void *user_data;
    PFN_wsiConfigureWindow pfn_configure;
    PFN_wsiCloseWindow     pfn_close;
};

void
wsi_window_handle_output_destroyed(struct wsi_window *w, struct wsi_output *o);

WsiExtent
wsi_window_get_geometry_extent(struct wsi_window *window);

WsiExtent
wsi_window_get_surface_extent(struct wsi_window *window);

WsiExtent
wsi_window_get_buffer_extent(struct wsi_window *window);

#endif
