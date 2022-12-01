#ifndef WSI_SRC_XCB_INPUT_PRIVATE_H
#define WSI_SRC_XCB_INPUT_PRIVATE_H

#include "wsi/input.h"

struct wsi_pointer {
    xcb_input_device_id_t device_id;
};

struct wsi_keyboard {
    xcb_input_device_id_t device_id;
};

#endif
