#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <poll.h>
#include <errno.h>

#include <wayland-client-protocol.h>
#include <viewporter-client-protocol.h>
#include <fractional-scale-v1-client-protocol.h>
#include <input-timestamps-unstable-v1-client-protocol.h>
#include <relative-pointer-unstable-v1-client-protocol.h>
#include <pointer-constraints-unstable-v1-client-protocol.h>
#include <xdg-shell-client-protocol.h>
#include <xdg-output-unstable-v1-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>

#include <xkbcommon/xkbcommon.h>

#include "platform_priv.h"
#include "input_priv.h"
#include "output_priv.h"

const uint32_t WSI_WL_COMPOSITOR_VERSION = 5;
const uint32_t WSI_WL_SHM_VERSION = 1;
const uint32_t WSI_WP_VIEWPORTER_VERSION = 1;
const uint32_t WSI_WP_FRACTIONAL_SCALE_MANAGER_V1_VERSION = 1;
const uint32_t WSI_WP_INPUT_TIMESTAMPS_MANAGER_V1_VERSION = 1;
const uint32_t WSI_WP_RELATIVE_POINTER_MANAGER_V1_VERSION = 1;
const uint32_t WSI_WP_POINTER_CONSTRAINTS_V1_VERSION = 1;
const uint32_t WSI_XDG_WM_BASE_VERSION = 5;
const uint32_t WSI_XDG_OUTPUT_MANAGER_V1_VERSION = 3;
const uint32_t WSI_XDG_DECORATION_MANAGER_V1_VERSION = 1;

uint64_t
wsi_new_id(struct wsi_platform *platform)
{
    uint64_t id = ++platform->id;
    assert(id != 0);
    return id;
}

uint32_t
wsi_get_version(const struct wl_interface *interface, uint32_t version, uint32_t max)
{
    if (version > max) {
        version = max;
    }
    if (version > (uint32_t)interface->version) {
        version = (uint32_t)interface->version;
    }
    return version;
}

static struct wsi_global *
wsi_global_create(struct wsi_platform *platform, uint32_t name, uint32_t version)
{
    struct wsi_global *global = calloc(1, sizeof(struct wsi_global));
    if (!global) {
        return NULL;
    }

    global->platform = platform;
    global->id = wsi_new_id(platform);
    global->name = name;
    global->version = version;

    return global;
}

static void
wsi_global_destroy(struct wsi_global *global)
{
    assert(global);
    free(global);
}

int
wsi_flush(struct wl_display *display)
{
    while (wl_display_flush(display) < 0) {
        if (errno == EINTR) {
            continue;
        }

        if (errno != EAGAIN) {
            return -1;
        }

        struct pollfd fds[1];
        fds[0].fd = wl_display_get_fd(display);
        fds[0].events = POLLOUT;

        int n;
        do {
            n = poll(fds, 1, -1);
        } while (n < 0 && errno == EINTR);

        if (n < 0) {
            return n;
        }
    }

    return 0;
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
    if (platform->wp_viewporter) {
        global = wp_viewporter_get_user_data(platform->wp_viewporter);
        wsi_global_destroy(global);
        wp_viewporter_destroy(platform->wp_viewporter);
    }
    if (platform->wp_fractional_scale_manager_v1) {
        global = wp_fractional_scale_manager_v1_get_user_data(platform->wp_fractional_scale_manager_v1);
        wsi_global_destroy(global);
        wp_fractional_scale_manager_v1_destroy(platform->wp_fractional_scale_manager_v1);
    }
    if (platform->wp_input_timestamps_manager_v1) {
        global = zwp_input_timestamps_manager_v1_get_user_data(platform->wp_input_timestamps_manager_v1);
        wsi_global_destroy(global);
        zwp_input_timestamps_manager_v1_destroy(platform->wp_input_timestamps_manager_v1);
    }
    if (platform->wp_relative_pointer_manager_v1) {
        global = zwp_relative_pointer_manager_v1_get_user_data(platform->wp_relative_pointer_manager_v1);
        wsi_global_destroy(global);
        zwp_relative_pointer_manager_v1_destroy(platform->wp_relative_pointer_manager_v1);
    }
    if (platform->wp_pointer_constraints_v1) {
        global = zwp_pointer_constraints_v1_get_user_data(platform->wp_pointer_constraints_v1);
        wsi_global_destroy(global);
        zwp_pointer_constraints_v1_destroy(platform->wp_pointer_constraints_v1);
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

static void *
wsi_global_bind(
    struct wsi_platform *platform,
    uint32_t name,
    const struct wl_interface *wl_interface,
    const void *listener,
    uint32_t version,
    uint32_t max_version)
{
    struct wsi_global *global = wsi_global_create(platform, name, version);
    if (!global) {
        return NULL;
    }

    version = wsi_get_version(wl_interface, version, max_version);

    struct wl_proxy *proxy = wl_registry_bind(platform->wl_registry, name, wl_interface, version);
    if (!proxy) {
        wsi_global_destroy(global);
        return NULL;
    }

    if (listener) {
        assert(wl_interface->event_count > 0);
        wl_proxy_add_listener(proxy, (void (**)(void))listener, global);
    } else {
        wl_proxy_set_user_data(proxy, global);
    }

    return proxy;
}

// region XDG WmBase

static void
xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

// endregion

// region WL Shm

static void
wl_shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    struct wsi_global *global = data;
    struct wsi_platform *platform = global->platform;

    uint32_t *next = wl_array_add(&platform->format_array, sizeof(uint32_t));
    if (!next) {
        return;
    }

    *next = format;
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
        platform->wl_compositor = wsi_global_bind(
            platform,
            name,
            &wl_compositor_interface,
            NULL,
            version,
            WSI_WL_COMPOSITOR_VERSION);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0) {
        platform->wl_shm = wsi_global_bind(
            platform,
            name,
            &wl_shm_interface,
            &wl_shm_listener,
            version,
            WSI_WL_SHM_VERSION);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0) {
        wsi_seat_add(platform, name, version);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0) {
        wsi_output_bind(platform, name, version);
    }
    else if (strcmp(interface, wp_viewporter_interface.name) == 0) {
        platform->wp_viewporter = wsi_global_bind(
            platform,
            name,
            &wp_viewporter_interface,
            NULL,
            version,
            WSI_WP_VIEWPORTER_VERSION);
    }
    else if (strcmp(interface, wp_fractional_scale_manager_v1_interface.name) == 0) {
        platform->wp_fractional_scale_manager_v1 = wsi_global_bind(
            platform,
            name,
            &wp_fractional_scale_manager_v1_interface,
            NULL,
            version,
            WSI_WP_FRACTIONAL_SCALE_MANAGER_V1_VERSION);
    }
    else if (strcmp(interface, zwp_input_timestamps_manager_v1_interface.name) == 0) {
        platform->wp_input_timestamps_manager_v1 = wsi_global_bind(
            platform,
            name,
            &zwp_input_timestamps_manager_v1_interface,
            NULL,
            version,
            WSI_WP_INPUT_TIMESTAMPS_MANAGER_V1_VERSION);
    }
    else if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0) {
        platform->wp_relative_pointer_manager_v1 = wsi_global_bind(
            platform,
            name,
            &zwp_relative_pointer_manager_v1_interface,
            NULL,
            version,
            WSI_WP_RELATIVE_POINTER_MANAGER_V1_VERSION);
    }
    else if (strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0) {
        platform->wp_pointer_constraints_v1 = wsi_global_bind(
            platform,
            name,
            &zwp_pointer_constraints_v1_interface,
            NULL,
            version,
            WSI_WP_POINTER_CONSTRAINTS_V1_VERSION);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        platform->xdg_wm_base = wsi_global_bind(
            platform,
            name,
            &xdg_wm_base_interface,
            &xdg_wm_base_listener,
            version,
            WSI_XDG_WM_BASE_VERSION);
    }
    else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
        platform->xdg_decoration_manager_v1 = wsi_global_bind(
            platform,
            name,
            &zxdg_decoration_manager_v1_interface,
            NULL,
            version,
            WSI_XDG_DECORATION_MANAGER_V1_VERSION);
    }
    else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
        platform->xdg_output_manager_v1 = wsi_global_bind(
            platform,
            name,
            &zxdg_output_manager_v1_interface,
            NULL,
            version,
            WSI_XDG_OUTPUT_MANAGER_V1_VERSION);
        if (platform->xdg_output_manager_v1) {
            wsi_output_init_xdg_all(platform);
        }
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
        if (seat->global.name== name) {
            wsi_seat_remove(seat);
            return;
        }
    }

    struct wsi_output *output, *output_tmp;
    wl_list_for_each_safe(output, output_tmp, &platform->output_list, link) {
        if (name == output->global.name) {
            wsi_output_destroy(output);
            return;
        }
    }

    struct wsi_global *global;

    global = wp_viewporter_get_user_data(platform->wp_viewporter);
    if (global->name == name) {
        wp_viewporter_destroy(platform->wp_viewporter);
        platform->wp_viewporter = NULL;
        wsi_global_destroy(global);
        return;
    }

    global = wp_fractional_scale_manager_v1_get_user_data(platform->wp_fractional_scale_manager_v1);
    if (global->name == name) {
        wp_fractional_scale_manager_v1_destroy(platform->wp_fractional_scale_manager_v1);
        platform->wp_fractional_scale_manager_v1 = NULL;
        wsi_global_destroy(global);
        return;
    }

    global = zwp_input_timestamps_manager_v1_get_user_data(platform->wp_input_timestamps_manager_v1);
    if (global->name == name) {
        zwp_input_timestamps_manager_v1_destroy(platform->wp_input_timestamps_manager_v1);
        platform->wp_input_timestamps_manager_v1 = NULL;
        wsi_global_destroy(global);
        return;
    }

    global = zwp_relative_pointer_manager_v1_get_user_data(platform->wp_relative_pointer_manager_v1);
    if (global->name == name) {
        zwp_relative_pointer_manager_v1_destroy(platform->wp_relative_pointer_manager_v1);
        platform->wp_relative_pointer_manager_v1 = NULL;
        wsi_global_destroy(global);
        return;
    }

    global = zwp_pointer_constraints_v1_get_user_data(platform->wp_pointer_constraints_v1);
    if (global->name == name) {
        zwp_pointer_constraints_v1_destroy(platform->wp_pointer_constraints_v1);
        platform->wp_pointer_constraints_v1 = NULL;
        wsi_global_destroy(global);
        return;
    }

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

static WsiResult
wsi_platform_init(const WsiPlatformCreateInfo *info, struct wsi_platform *platform)
{
    WsiResult result;

    platform->wl_display = wl_display_connect(NULL);
    if (platform->wl_display == NULL) {
        result = WSI_ERROR_PLATFORM;
        goto err_display;
    }

    wl_list_init(&platform->seat_list);
    wl_list_init(&platform->output_list);
    wl_list_init(&platform->window_list);
    wl_array_init(&platform->format_array);

    platform->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (platform->xkb_context == NULL) {
        result = WSI_ERROR_PLATFORM;
        goto err_xkb;
    }

    platform->wl_registry = wl_display_get_registry(platform->wl_display);
    if (platform->wl_registry == NULL) {
        result = WSI_ERROR_OUT_OF_MEMORY;
        goto err_registry;
    }

    wl_registry_add_listener(platform->wl_registry, &wl_registry_listener, platform);

    wl_display_roundtrip(platform->wl_display);

    if (!platform->wl_compositor) {
        result = WSI_ERROR_PLATFORM;
        goto err_globals;
    }
    if (!platform->wl_shm) {
        result = WSI_ERROR_PLATFORM;
        goto err_globals;
    }

    return WSI_SUCCESS;

err_globals:
    wsi_seat_destroy_all(platform);
    wsi_output_destroy_all(platform);
    wsi_platform_destroy_globals(platform);
    wl_registry_destroy(platform->wl_registry);
    wl_display_roundtrip(platform->wl_display);
err_registry:
    xkb_context_unref(platform->xkb_context);
err_xkb:
    wl_array_release(&platform->format_array);
    wl_display_disconnect(platform->wl_display);
err_display:
    return result;
}

static void
wsi_platform_uninit(struct wsi_platform *platform)
{
    wsi_seat_destroy_all(platform);
    wsi_output_destroy_all(platform);
    wsi_platform_destroy_globals(platform);
    wl_registry_destroy(platform->wl_registry);
    wl_display_roundtrip(platform->wl_display);
    xkb_context_unref(platform->xkb_context);
    wl_array_release(&platform->format_array);
    wl_display_disconnect(platform->wl_display);
}

WsiResult
wsiCreatePlatform(const WsiPlatformCreateInfo *pCreateInfo, WsiPlatform *pPlatform)
{
    struct wsi_platform *platform = calloc(1, sizeof(*platform));
    if (platform == NULL) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    WsiResult result = wsi_platform_init(pCreateInfo, platform);
    if (result != WSI_SUCCESS) {
        free(platform);
        return result;
    }

    *pPlatform = platform;
    return WSI_SUCCESS;
}

void
wsiDestroyPlatform(WsiPlatform platform)
{
    wsi_platform_uninit(platform);
    free(platform);
}

WsiResult
wsiDispatchEvents(WsiPlatform platform, int64_t timeout)
{
    struct wl_display *wl_display = platform->wl_display;

    if (wl_display_prepare_read(wl_display) < 0) {
        int n = wl_display_dispatch_pending(wl_display);
        return n < 0 ? WSI_ERROR_PLATFORM : WSI_SUCCESS;
    }

    int n = wsi_flush(wl_display);
    if (n < 0) {
        wl_display_cancel_read(wl_display);
        if (errno == ENOMEM) {
            return WSI_ERROR_OUT_OF_MEMORY;
        }
        return WSI_ERROR_PLATFORM;
    }

    struct pollfd fds[1];
    fds[0].fd = wl_display_get_fd(wl_display);
    fds[0].events = POLLIN;

    do {
        n = poll(fds, 1, 0);
    } while (n < 0 && errno == EINTR);

    if (n < 0) {
        wl_display_cancel_read(wl_display);
        if (errno == ENOMEM) {
            return WSI_ERROR_OUT_OF_MEMORY;
        }
        return WSI_ERROR_PLATFORM;
    }

    if (fds[0].revents & POLLIN) {
        n = wl_display_read_events(wl_display);
        if (n < 0) {
            return WSI_ERROR_PLATFORM;
        }
    } else {
        wl_display_cancel_read(wl_display);
    }

    n = wl_display_dispatch_pending(wl_display);
    if (n < 0) {
        return WSI_ERROR_PLATFORM;
    }

    return WSI_SUCCESS;
}
