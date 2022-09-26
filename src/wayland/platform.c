#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <poll.h>

#include <wayland-client-protocol.h>
#include <xdg-shell-client-protocol.h>
#include <xdg-output-unstable-v1-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>

#include "wsi/platform.h"

#include "platform_priv.h"
#include "seat_priv.h"
#include "output_priv.h"

struct wsi_global *
wsi_global_create(struct wsi_platform *platform, uint32_t name)
{
    struct wsi_global *global = calloc(1, sizeof(struct wsi_global));
    if (!global) {
        return NULL;
    }

    global->platform = platform;
    global->name = name;

    return global;
}

void
wsi_global_destroy(struct wsi_global *global)
{
    assert(global);
    free(global);
}

static void
wsi_platform_destroy_globals(struct wsi_platform *platform)
{
    struct wsi_global *global;
    if (platform->wl_compositor) {
        global = wl_compositor_get_user_data(platform->wl_compositor);
        wsi_global_destroy(global);
        wl_compositor_destroy(platform->wl_compositor);
    }
    if (platform->wl_shm) {
        global = wl_shm_get_user_data(platform->wl_shm);
        wsi_global_destroy(global);
        wl_shm_destroy(platform->wl_shm);
    }
    if (platform->xdg_wm_base) {
        global = xdg_wm_base_get_user_data(platform->xdg_wm_base);
        wsi_global_destroy(global);
        xdg_wm_base_destroy(platform->xdg_wm_base);
    }
    if (platform->xdg_decoration_manager_v1) {
        global = zxdg_decoration_manager_v1_get_user_data(
            platform->xdg_decoration_manager_v1);
        wsi_global_destroy(global);
        zxdg_decoration_manager_v1_destroy(platform->xdg_decoration_manager_v1);
    }
    if (platform->xdg_output_manager_v1) {
        global = zxdg_output_manager_v1_get_user_data(
            platform->xdg_output_manager_v1);
        wsi_global_destroy(global);
        zxdg_output_manager_v1_destroy(platform->xdg_output_manager_v1);
    }
}

void *
wsi_platform_bind(
    struct wsi_platform *platform,
    uint32_t name,
    const struct wl_interface *wl_interface,
    uint32_t version)
{
    uint32_t v = wl_interface->version < version
               ? wl_interface->version
               : version;

    return wl_registry_bind(
        platform->wl_registry,
        name,
        wl_interface,
        v);
}

// region XDG WmBase

static void
xdg_wm_base_ping(
    void *data,
    struct xdg_wm_base *xdg_wm_base,
    uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

// endregion

// region WL Shm

static void
wl_shm_format(
    void *data,
    struct wl_shm *wl_shm,
    uint32_t format)
{

}

static const struct wl_shm_listener wl_shm_listener = {
    .format = wl_shm_format,
};

// endregion

// region WL Registry

static void
wl_registry_global(
    void *data,
    struct wl_registry *wl_registry,
    uint32_t name,
    const char *interface,
    uint32_t version)
{
    struct wsi_platform *platform = data;
    assert(platform->wl_registry = wl_registry);

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        struct wsi_global *global = wsi_global_create(platform, name);
        platform->wl_compositor = wsi_platform_bind(
            platform,
            name,
            &wl_compositor_interface,
            version);
        wl_compositor_set_user_data(platform->wl_compositor, global);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0) {
        struct wsi_global *global = wsi_global_create(platform, name);
        platform->wl_shm = wsi_platform_bind(
            platform,
            name,
            &wl_shm_interface,
            version);
        wl_shm_add_listener(platform->wl_shm, &wl_shm_listener, global);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0) {
        wsi_seat_bind(platform, name, version);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0) {
        wsi_output_bind(platform, name, version);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        struct wsi_global *global = wsi_global_create(platform, name);
        platform->xdg_wm_base = wsi_platform_bind(
            platform,
            name,
            &xdg_wm_base_interface,
            version);
        xdg_wm_base_add_listener(
            platform->xdg_wm_base,
            &xdg_wm_base_listener,
            global);
    }
    else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
        struct wsi_global *global = wsi_global_create(platform, name);
        platform->xdg_decoration_manager_v1 = wsi_platform_bind(
            platform,
            name,
            &zxdg_decoration_manager_v1_interface,
            version);
        zxdg_decoration_manager_v1_set_user_data(
            platform->xdg_decoration_manager_v1,
            global);
    }
    else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
        struct wsi_global *global = wsi_global_create(platform, name);
        platform->xdg_output_manager_v1 = wsi_platform_bind(
            platform,
            name,
            &zxdg_output_manager_v1_interface,
            version);
        zxdg_output_manager_v1_set_user_data(
            platform->xdg_output_manager_v1,
            global);
        wsi_output_init_xdg_all(platform);
    }
}

static void
wl_registry_global_remove(
    void *data,
    struct wl_registry *wl_registry,
    uint32_t name)
{
    struct wsi_platform *platform = data;
    assert(platform->wl_registry == wl_registry);

    struct wsi_seat *seat, *seat_tmp;
    wl_list_for_each_safe(seat, seat_tmp, &platform->seat_list, link) {
        if (name == seat->wl_global_name) {
            wsi_seat_destroy(seat);
            return;
        }
    }

    struct wsi_output *output, *output_tmp;
    wl_list_for_each_safe(output, output_tmp, &platform->output_list, link) {
        if (name == output->wl_global_name) {
            wsi_output_destroy(output);
            return;
        }
    }

    struct wsi_global *global;

    global = zxdg_decoration_manager_v1_get_user_data(
        platform->xdg_decoration_manager_v1);
    if (global->name == name) {
        zxdg_decoration_manager_v1_destroy(platform->xdg_decoration_manager_v1);
        platform->xdg_decoration_manager_v1 = NULL;
        wsi_global_destroy(global);
        return;
    }

    global = zxdg_output_manager_v1_get_user_data(
        platform->xdg_output_manager_v1);
    if (global->name == name) {
        zxdg_output_manager_v1_destroy(platform->xdg_output_manager_v1);
        platform->xdg_output_manager_v1 = NULL;
        wsi_global_destroy(global);
        return;
    }

    global = xdg_wm_base_get_user_data(platform->xdg_wm_base);
    if (global->name == name) {
        // TODO: Trigger a shutdown
        xdg_wm_base_destroy(platform->xdg_wm_base);
        wsi_global_destroy(global);
        return;
    }

    global = wl_compositor_get_user_data(platform->wl_compositor);
    if (global->name == name) {
        // TODO: Trigger a shutdown
        wl_compositor_destroy(platform->wl_compositor);
        wsi_global_destroy(global);
        return;
    }

    global = wl_shm_get_user_data(platform->wl_shm);
    if (global->name == name) {
        // TODO: Trigger a shutdown
        wl_shm_destroy(platform->wl_shm);
        wsi_global_destroy(global);
        return;
    }
}

static const struct wl_registry_listener wl_registry_listener = {
    .global        = wl_registry_global,
    .global_remove = wl_registry_global_remove,
};

// endregion

WsiResult
wsiCreatePlatform(WsiPlatform *pPlatform)
{
    enum wsi_result result;

    struct wsi_platform *platform = calloc(1, sizeof(struct wsi_platform));
    if (platform == NULL) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    platform->wl_display = wl_display_connect(NULL);
    if (platform->wl_display == NULL) {
        result = WSI_ERROR_PLATFORM;
        goto err_display;
    }

    wl_list_init(&platform->seat_list);
    wl_list_init(&platform->output_list);

    platform->wl_registry = wl_display_get_registry(platform->wl_display);
    wl_registry_add_listener(platform->wl_registry, &wl_registry_listener, platform);

    wl_display_roundtrip(platform->wl_display);

    if (!platform->wl_compositor) {
        result = WSI_ERROR_PLATFORM;
        goto err_registry;
    }
    if (!platform->wl_shm) {
        result = WSI_ERROR_PLATFORM;
        goto err_registry;
    }

    *pPlatform = platform;
    return WSI_SUCCESS;

err_registry:
    wsi_seat_destroy_all(platform);
    wsi_output_destroy_all(platform);
    wsi_platform_destroy_globals(platform);
    wl_registry_destroy(platform->wl_registry);
    wl_display_roundtrip(platform->wl_display);
    wl_display_disconnect(platform->wl_display);
err_display:
    free(platform);
    return result;
}

void
wsiDestroyPlatform(WsiPlatform platform)
{
    wsi_seat_destroy_all(platform);
    wsi_output_destroy_all(platform);

    wsi_platform_destroy_globals(platform);

    wl_registry_destroy(platform->wl_registry);

    wl_display_roundtrip(platform->wl_display);

    wl_display_disconnect(platform->wl_display);

    free(platform);
}

void
wsiGetPlatformFeatures(WsiPlatform platform, WsiPlatformFeatures *pFeatures)
{
    memset(pFeatures, 0, sizeof(WsiPlatformFeatures));

    pFeatures->windowing = platform->xdg_wm_base != NULL;
}

void
wsiGetPlatformLimits(WsiPlatform platform, WsiPlatformLimits *pLimits)
{
    memset(pLimits, 0, sizeof(WsiPlatformLimits));

    pLimits->maxWindowWidth = INT32_MAX;
    pLimits->maxWindowHeight = INT32_MAX;
    pLimits->maxEventQueueCount = 0;
    pLimits->maxSeatCount = 0;
}

void
wsiPoll(WsiPlatform platform)
{
    struct wl_display *wl_display = platform->wl_display;

    while (wl_display_prepare_read(wl_display) == -1) {
        wl_display_dispatch_pending(wl_display);
    }

    wl_display_flush(wl_display);

    struct pollfd pfd = {
        .fd = wl_display_get_fd(wl_display),
        .events = POLLIN,
        .revents = 0,
    };

    int n = poll(&pfd, 1, 0);
    if (n == -1) {
        wl_display_cancel_read(wl_display);
        return;
    }

    if (pfd.revents & POLLIN) {
        wl_display_read_events(wl_display);
    } else {
        wl_display_cancel_read(wl_display);
    }

    wl_display_dispatch_pending(wl_display);
}
