#ifndef WSI_SRC_WAYLAND_EGL_EGL_PRIVATE_H
#define WSI_SRC_WAYLAND_EGL_EGL_PRIVATE_H

struct wsi_egl;

enum wsi_result
wsi_egl_load(
    struct wsi_platform *platform);

void
wsi_egl_unload(
    struct wsi_platform *platform);

void
wsi_window_egl_configure(
    struct wsi_window *window,
    struct wsi_xdg_extent extent);

#endif
