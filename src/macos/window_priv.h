#ifndef WSI_SRC_COCOA_WINDOW_PRIVATE_H
#define WSI_SRC_COCOA_WINDOW_PRIVATE_H

#include "wsi/window.h"

#include "common_priv.h"

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
typedef void* id;
#endif

struct wsi_window {
    struct wsi_platform *platform;

    enum wsi_api api;

    id window;
    id windowDelegate;
    id view;
    id layer;

    uint16_t user_width;
    uint16_t user_height;

    void *user_data;
    PFN_wsiConfigureWindow pfn_configure;
    PFN_wsiCloseWindow pfn_close;
};

#endif
