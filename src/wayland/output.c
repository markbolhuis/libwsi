#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <wayland-client-protocol.h>
#include <xdg-output-unstable-v1-client-protocol.h>

#include "wsi/output.h"
#include "wsi/window.h"

#include "common_priv.h"
#include "platform_priv.h"
#include "output_priv.h"
#include "window_priv.h"

#define WSI_WL_OUTPUT_VERSION 4
#define WSI_XDG_OUTPUT_V1_DONE_DEPRECATED_SINCE_VERSION 3

int32_t
wsi_output_get_scale(struct wsi_output *output)
{
    if (wl_output_get_version(output->wl_output) <
        WL_OUTPUT_SCALE_SINCE_VERSION)
    {
        return 1;
    }

    return output->scale;
}

static int
wsi_output_get_required_done_count(struct wsi_output *output)
{
    int count = 0;

    uint32_t wl_ver = wl_output_get_version(output->wl_output);
    if (wl_ver >= WL_OUTPUT_DONE_SINCE_VERSION) {
        count++;
    }

    if (output->xdg_output_v1 == NULL) {
        return count;
    }

    uint32_t xdg_ver = zxdg_output_v1_get_version(output->xdg_output_v1);
    if (xdg_ver < WSI_XDG_OUTPUT_V1_DONE_DEPRECATED_SINCE_VERSION ||
        wl_ver >= WL_OUTPUT_DONE_SINCE_VERSION) {
        count++;
    }

    return count;
}

static void
wsi_output_done(struct wsi_output *output)
{
    int required = wsi_output_get_required_done_count(output);
    if (output->done_count < required) {
        return;
    }

    // TODO: handle atomic update
}

// region XDG Output V1

static void
xdg_output_v1_logical_position(
    void *data,
    struct zxdg_output_v1 *xdg_output_v1,
    int32_t x,
    int32_t y)
{
    struct wsi_output *output = data;

    output->x = x;
    output->y = y;
}

static void
xdg_output_v1_logical_size(
    void *data,
    struct zxdg_output_v1 *xdg_output_v1,
    int32_t width,
    int32_t height)
{
    struct wsi_output *output = data;

    output->logical_width = width;
    output->logical_height = height;
}

static void
xdg_output_v1_done(
    void *data,
    struct zxdg_output_v1 *xdg_output_v1)
{
    struct wsi_output *output = data;

    // Catch any compositor bug, and prevent it messing up the done count
    if (zxdg_output_v1_get_version(xdg_output_v1) >=
        WSI_XDG_OUTPUT_V1_DONE_DEPRECATED_SINCE_VERSION)
    {
        return;
    }

    output->done_count++;
    wsi_output_done(output);
}

static void
xdg_output_v1_name(
    void *data,
    struct zxdg_output_v1 *xdg_output_v1,
    const char *name)
{
    struct wsi_output *output = data;

    free(output->name);
    output->name = strdup(name);
}

static void
xdg_output_v1_description(
    void *data,
    struct zxdg_output_v1 *xdg_output_v1,
    const char *description)
{
    struct wsi_output *output = data;

    free(output->description);
    output->description = strdup(description);
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
    struct wsi_output *output = data;

    if (output->xdg_output_v1 == NULL) {
        output->x = x;
        output->y = y;
    }

    output->physical_width = physical_width;
    output->physical_height = physical_height;
    output->subpixel = subpixel;

    free(output->make);
    output->make = strdup(make);

    free(output->model);
    output->model = strdup(model);

    output->transform = transform;
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
    struct wsi_output *output = data;

    if (flags != WL_OUTPUT_MODE_CURRENT) {
        return;
    }

    output->pixel_width = width;
    output->pixel_height = height;

    output->refresh = refresh;
}

static void
wl_output_done(
    void *data,
    struct wl_output *wl_output)
{
    struct wsi_output *output = data;
    if (output->xdg_output_v1) {
        output->done_count++;
    } else {
        output->done_count = 1;
    }
    wsi_output_done(output);
}

static void
wl_output_scale(
    void *data,
    struct wl_output *wl_output,
    int32_t scale)
{
    struct wsi_output *output = data;
    output->scale = scale;
}

static void
wl_output_name(
    void *data,
    struct wl_output *wl_output,
    const char *name)
{
    struct wsi_output *output = data;

    if (output->xdg_output_v1 &&
        zxdg_output_v1_get_version(output->xdg_output_v1) >=
            ZXDG_OUTPUT_V1_NAME_SINCE_VERSION)
    {
        return;
    }

    free(output->name);
    output->name = strdup(name);
}

static void
wl_output_description(
    void *data,
    struct wl_output *wl_output,
    const char *description)
{
    struct wsi_output *output = data;

    if (output->xdg_output_v1 &&
        zxdg_output_v1_get_version(output->xdg_output_v1) >=
            ZXDG_OUTPUT_V1_DESCRIPTION_SINCE_VERSION)
    {
        return;
    }

    free(output->description);
    output->description = strdup(description);
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
wsi_output_init_xdg(struct wsi_output *output)
{
    struct wsi_platform *platform = output->global.platform;

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
            wsi_output_init_xdg(output);
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

    output->global.platform = platform;
    output->global.name = name;

    output->wl_output = wsi_bind(
        platform,
        name,
        &wl_output_interface,
        version,
        WSI_WL_OUTPUT_VERSION);
    wl_output_add_listener(
        output->wl_output,
        &wl_output_listener,
        output);

    if (platform->xdg_output_manager_v1 != NULL) {
        wsi_output_init_xdg(output);
    }

    wl_list_insert(&platform->output_list, &output->link);
    return output;
}

void
wsi_output_destroy(struct wsi_output *output)
{
    struct wsi_platform *platform = output->global.platform;

    struct wsi_window *window;
    wl_list_for_each(window, &platform->window_list, link) {
        wsi_window_handle_output_destroyed(window, output);
    }

    if (output->xdg_output_v1) {
        zxdg_output_v1_destroy(output->xdg_output_v1);
    }

    if (wl_output_get_version(output->wl_output) >=
        WL_OUTPUT_RELEASE_SINCE_VERSION)
    {
        wl_output_release(output->wl_output);
    } else {
        wl_output_destroy(output->wl_output);
    }

    free(output->name);
    free(output->description);
    free(output->make);
    free(output->model);

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
