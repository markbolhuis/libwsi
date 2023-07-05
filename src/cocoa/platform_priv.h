#ifndef WSI_SRC_COCOA_PLATFORM_PRIVATE_H
#define WSI_SRC_COCOA_PLATFORM_PRIVATE_H

#include "wsi/platform.h"

struct wsi_platform {
    //TODO

    struct wsi_list  window_list;
};

#define wsi_array_length(array) (sizeof(array) / sizeof((array)[0]))

#endif
