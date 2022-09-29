#ifndef WSI_SRC_XCB_WINDOW_PRIVATE_H
#define WSI_SRC_XCB_WINDOW_PRIVATE_H

struct wsi_xcb_extent {
    uint16_t width;
    uint16_t height;
};

struct wsi_window {
    struct wsi_platform *platform;

    enum wsi_api api;

    xcb_window_t xcb_window;
    xcb_window_t xcb_parent;

    struct wsi_xcb_extent user_extent;
};

#endif
