#ifndef WSI_SRC_XCB_PLATFORM_PRIVATE_H
#define WSI_SRC_XCB_PLATFORM_PRIVATE_H

#include "wsi/platform.h"

struct wsi_event_queue {
    struct wsi_platform *platform;
    PFN_wsiEventCallback pfn_callback;
    void *user_data;
};

struct wsi_platform {
    xcb_connection_t *xcb_connection;
    xcb_screen_t     *xcb_screen;
    int              xcb_screen_id;

    struct wsi_event_queue queue;

    xcb_atom_t       xcb_atom_wm_protocols;
    xcb_atom_t       xcb_atom_wm_delete_window;

    struct wsi_list  window_list;
};

#define wsi_array_length(array) (sizeof(array) / sizeof((array)[0]))

#endif
