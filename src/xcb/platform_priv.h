#ifndef WSI_SRC_XCB_PLATFORM_PRIVATE_H
#define WSI_SRC_XCB_PLATFORM_PRIVATE_H

struct wsi_platform {
    xcb_connection_t *xcb_connection;
    xcb_screen_t     *xcb_screen;
    int              xcb_screen_id;
};

#endif
