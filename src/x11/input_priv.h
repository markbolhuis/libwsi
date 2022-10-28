#ifndef WSI_SRC_XCB_INPUT_PRIVATE_H
#define WSI_SRC_XCB_INPUT_PRIVATE_H

struct wsi_pointer {
    xcb_input_device_id_t device_id;
};

struct wsi_keyboard {
    xcb_input_device_id_t device_id;
};

#endif
