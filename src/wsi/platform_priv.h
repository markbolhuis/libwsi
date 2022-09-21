#ifndef WSI_SRC_WSI_HANDLE_H
#define WSI_SRC_WSI_HANDLE_H

struct wsi_platform {
    void *handle;
    struct wsi_platform *platform;
};

void *
wsi_platform_dlsym(struct wsi_platform *platform, const char *symbol);

#endif
