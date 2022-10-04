#ifndef WSI_SRC_WSI_OUTPUT_PRIVATE_H
#define WSI_SRC_WSI_OUTPUT_PRIVATE_H

struct wsi_output {
    struct wsi_platform *platform;
    struct wsi_output *output;
};

void *
wsi_output_dlsym(struct wsi_output *output, const char *symbol);

#endif
