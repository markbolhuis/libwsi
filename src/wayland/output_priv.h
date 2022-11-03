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
    int32_t logical_width;
    int32_t logical_height;
    int32_t pixel_width;
    int32_t pixel_height;
    int32_t physical_width;
    int32_t physical_height;
    int32_t scale;
    int32_t subpixel;
    int32_t transform;
    int32_t refresh;

    char *name;
    char *description;
    char *make;
    char *model;
};

int32_t
wsi_output_get_scale(struct wsi_output *output);

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
