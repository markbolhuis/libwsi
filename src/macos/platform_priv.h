#ifndef WSI_SRC_COCOA_PLATFORM_PRIVATE_H
#define WSI_SRC_COCOA_PLATFORM_PRIVATE_H

#include "wsi/platform.h"

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
typedef void* id;
#endif

struct wsi_platform {
    id appDelegate;
};

#define wsi_array_length(array) (sizeof(array) / sizeof((array)[0]))

#endif
