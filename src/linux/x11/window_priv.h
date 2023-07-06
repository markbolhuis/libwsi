#ifndef WSI_SRC_XCB_WINDOW_PRIVATE_H
#define WSI_SRC_XCB_WINDOW_PRIVATE_H

#include "wsi/window.h"

struct wsi_window {
    struct wsi_platform *platform;

    struct wsi_list link;

    xcb_window_t xcb_window;
    xcb_window_t xcb_parent;
    xcb_colormap_t xcb_colormap;

    uint16_t user_width;
    uint16_t user_height;

    void *user_data;
    PFN_wsiConfigureWindow pfn_configure;
    PFN_wsiCloseWindow pfn_close;
};

void
wsi_window_xcb_configure_notify(
    struct wsi_window *window,
    const xcb_configure_notify_event_t *event);

void
wsi_window_xcb_client_message(
    struct wsi_window *window,
    const xcb_client_message_event_t *event);

#endif
