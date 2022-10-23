#ifndef WSI_SRC_WSI_INPUT_PRIVATE_H
#define WSI_SRC_WSI_INPUT_PRIVATE_H

struct wsi_pointer {
    struct wsi_platform *platform;
    struct wsi_pointer *pointer;
};

struct wsi_keyboard {
    struct wsi_platform *platform;
    struct wsi_keyboard *keyboard;
};

void *
wsi_pointer_dlsym(struct wsi_pointer *pointer, const char *symbol);

void *
wsi_keyboard_dlsym(struct wsi_keyboard *keyboard, const char *symbol);

#endif
