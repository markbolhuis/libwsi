#ifndef WSI_SRC_WSI_WINDOW_PRIVATE_H
#define WSI_SRC_WSI_WINDOW_PRIVATE_H

struct wsi_window {
    struct wsi_platform *platform;
    struct wsi_window *window;
};

void *
wsi_window_dlsym(struct wsi_window *window, const char *symbol);

#endif
