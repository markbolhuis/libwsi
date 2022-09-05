#ifndef WSI_SRC_XCB_WINDOW_PRIVATE_H
#define WSI_SRC_XCB_WINDOW_PRIVATE_H

struct wsi_window {
    struct wsi_platform *platform;

    xcb_window_t xcb_window;
    uint16_t width;
    uint16_t height;
};

#endif
