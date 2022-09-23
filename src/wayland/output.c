#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <wayland-client-protocol.h>
#include <xdg-output-unstable-v1-client-protocol.h>

#include "wsi/output.h"

#include "platform_priv.h"
#include "output_priv.h"

// region XDG Output V1

static void
xdg_output_v1_logical_position(
    void *data,
    struct zxdg_output_v1 *xdg_output_v1,
    int32_t x,
    int32_t y)
{

}

static void
xdg_output_v1_logical_size(
    void *data,
    struct zxdg_output_v1 *xdg_output_v1,
    int32_t width,
    int32_t height)
{

}

static void
xdg_output_v1_done(
    void *data,
    struct zxdg_output_v1 *xdg_output_v1)
{

}

static void
xdg_output_v1_name(
    void *data,
    struct zxdg_output_v1 *xdg_output_v1,
    const char *name)
{

}

static void
xdg_output_v1_description(
    void *data,
    struct zxdg_output_v1 *xdg_output_v1,
    const char *description)
{

}

static const struct zxdg_output_v1_listener xdg_output_v1_listener = {
    .logical_position = xdg_output_v1_logical_position,
    .logical_size     = xdg_output_v1_logical_size,
    .done             = xdg_output_v1_done,
    .name             = xdg_output_v1_name,
    .description      = xdg_output_v1_description,
};

// endregion

// region WL Output

static void
wl_output_geometry(
    void *data,
    struct wl_output *wl_output,
    int32_t x,
    int32_t y,
    int32_t physical_width,
    int32_t physical_height,
    int32_t subpixel,
    const char *make,
    const char *model,
    int32_t transform)
{
}

static void
wl_output_mode(
    void *data,
    struct wl_output *wl_output,
    uint32_t flags,
    int32_t width,
    int32_t height,
    int32_t refresh)
{
}

static void
wl_output_done(
    void *data,
    struct wl_output *wl_output)
{
}

static void
wl_output_scale(
    void *data,
    struct wl_output *wl_output,
    int32_t scale)
{
}

static void
wl_output_name(
    void *data,
    struct wl_output *wl_output,
    const char *name)
{
}

static void
wl_output_description(
    void *data,
    struct wl_output *wl_output,
    const char *description)
{
}

static const struct wl_output_listener wl_output_listener = {
    .geometry    = wl_output_geometry,
    .mode        = wl_output_mode,
    .done        = wl_output_done,
    .scale       = wl_output_scale,
    .name        = wl_output_name,
    .description = wl_output_description,
};

// endregion

void
wsi_output_init_xdg(struct wsi_platform *platform, struct wsi_output *output)
{
    assert(platform->xdg_output_manager_v1 != NULL);
    assert(output->xdg_output_v1 == NULL);

    output->xdg_output_v1 = zxdg_output_manager_v1_get_xdg_output(
        platform->xdg_output_manager_v1,
        output->wl_output);
    zxdg_output_v1_add_listener(
        output->xdg_output_v1,
        &xdg_output_v1_listener,
        output);
}

void
wsi_output_init_xdg_all(struct wsi_platform *platform)
{
    assert(platform->xdg_output_manager_v1 != NULL);

    struct wsi_output *output;
    wl_list_for_each(output, &platform->output_list, link) {
        if (output->xdg_output_v1 == NULL) {
            wsi_output_init_xdg(platform, output);
        }
    }
}

struct wsi_output *
wsi_output_bind(struct wsi_platform *platform, uint32_t name, uint32_t version)
{
    struct wsi_output *output = calloc(1, sizeof(struct wsi_output));
    if (!output) {
        return NULL;
    }

    output->wl_output = wsi_platform_bind(
        platform,
        name,
        &wl_output_interface,
        version);
    wl_output_add_listener(
        output->wl_output,
        &wl_output_listener,
        output);

    if (platform->xdg_output_manager_v1 != NULL) {
        wsi_output_init_xdg(platform, output);
    }

    wl_list_insert(&platform->output_list, &output->link);
    return output;
}

void
wsi_output_destroy(struct wsi_output *output)
{
    assert(output->wl_output);

    if (output->xdg_output_v1) {
        zxdg_output_v1_destroy(output->xdg_output_v1);
    }

    if (wl_output_get_version(output->wl_output) >= WL_OUTPUT_RELEASE_SINCE_VERSION) {
        wl_output_release(output->wl_output);
    } else {
        wl_output_destroy(output->wl_output);
    }

    wl_list_remove(&output->link);
    free(output);
}

void
wsi_output_destroy_all(struct wsi_platform *platform)
{
    struct wsi_output *output, *output_tmp;
    wl_list_for_each_safe(output, output_tmp, &platform->output_list, link) {
        wsi_output_destroy(output);
    }
}

WsiResult
wsiCreateOutput(WsiPlatform platform, WsiOutput *pOutput)
{
    return WSI_ERROR_NOT_IMPLEMENTED;
}
