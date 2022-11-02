#ifndef WSI_SRC_WAYLAND_OUTPUT_PRIVATE_H
#define WSI_SRC_WAYLAND_OUTPUT_PRIVATE_H

struct wsi_output {
    struct wsi_global global;

    struct wl_list link;

    struct wl_output *wl_output;
    struct zxdg_output_v1 *xdg_output_v1;
    int done_count;

    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    int32_t scale;

    char *name;
    char *description;

};

void
wsi_output_init_xdg(struct wsi_output *output);

void
wsi_output_init_xdg_all(struct wsi_platform *platform);

struct wsi_output *
wsi_output_bind(struct wsi_platform *platform, uint32_t name, uint32_t version);

void
wsi_output_destroy(struct wsi_output *output);

void
wsi_output_destroy_all(struct wsi_platform *platform);

#endif
