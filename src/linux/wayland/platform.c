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
#include <pointer-gestures-unstable-v1-client-protocol.h>
#include <keyboard-shortcuts-inhibit-unstable-v1-client-protocol.h>
#include <idle-inhibit-unstable-v1-client-protocol.h>
#include <content-type-v1-client-protocol.h>
#include <xdg-shell-client-protocol.h>
#include <xdg-output-unstable-v1-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>
#include <ext-idle-notify-v1-client-protocol.h>

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
const uint32_t WSI_WP_POINTER_GESTURES_V1_VERSION = 3;
const uint32_t WSI_WP_KEYBOARD_SHORTCUTS_INHIBIT_MANAGER_V1_VERSION = 1;
const uint32_t WSI_WP_IDLE_INHIBIT_MANAGER_V1_VERSION = 1;
const uint32_t WSI_WP_CONTENT_TYPE_MANAGER_V1_VERSION = 1;
const uint32_t WSI_XDG_WM_BASE_VERSION = 6;
const uint32_t WSI_XDG_OUTPUT_MANAGER_V1_VERSION = 3;
const uint32_t WSI_XDG_DECORATION_MANAGER_V1_VERSION = 1;
const uint32_t WSI_EXT_IDLE_NOTIFICATION_V1_VERSION = 1;

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
#define WSI_GLOBAL_DESTROY(name) \
    if (platform->name != NULL) { \
        struct wsi_global *global = name##_get_user_data(platform->name); \
        wsi_global_destroy(global); \
        name##_destroy(platform->name); \
        platform->name = NULL; \
    }

#define ZWSI_GLOBAL_DESTROY(name) \
    if (platform->name != NULL) { \
        struct wsi_global *global = z##name##_get_user_data(platform->name); \
        wsi_global_destroy(global); \
        z##name##_destroy(platform->name); \
        platform->name = NULL; \
    }

    WSI_GLOBAL_DESTROY(wl_compositor)
    WSI_GLOBAL_DESTROY(wl_shm)
    WSI_GLOBAL_DESTROY(wp_viewporter)
    WSI_GLOBAL_DESTROY(wp_fractional_scale_manager_v1)
    WSI_GLOBAL_DESTROY(wp_content_type_manager_v1);
    WSI_GLOBAL_DESTROY(xdg_wm_base)
    WSI_GLOBAL_DESTROY(ext_idle_notifier_v1)

    ZWSI_GLOBAL_DESTROY(wp_input_timestamps_manager_v1)
    ZWSI_GLOBAL_DESTROY(wp_relative_pointer_manager_v1)
    ZWSI_GLOBAL_DESTROY(wp_pointer_constraints_v1)
    ZWSI_GLOBAL_DESTROY(wp_keyboard_shortcuts_inhibit_manager_v1)
    ZWSI_GLOBAL_DESTROY(wp_idle_inhibit_manager_v1)
    ZWSI_GLOBAL_DESTROY(xdg_decoration_manager_v1)
    ZWSI_GLOBAL_DESTROY(xdg_output_manager_v1)

    if (platform->wp_pointer_gestures_v1) {
        struct wsi_global *global = zwp_pointer_gestures_v1_get_user_data(
            platform->wp_pointer_gestures_v1);
        wsi_global_destroy(global);
        if (zwp_pointer_gestures_v1_get_version(platform->wp_pointer_gestures_v1) >=
            ZWP_POINTER_GESTURES_V1_RELEASE_SINCE_VERSION)
        {
            zwp_pointer_gestures_v1_release(platform->wp_pointer_gestures_v1);
        } else {
            zwp_pointer_gestures_v1_destroy(platform->wp_pointer_gestures_v1);
        }
        platform->wp_pointer_gestures_v1 = NULL;
    }

#undef WSI_GLOBAL_DESTROY
#undef ZWSI_GLOBAL_DESTROY
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

#define WSI_MATCH(ident) \
    ((strcmp(interface, ident##_interface.name) == 0) && (platform->ident == NULL))

#define ZWSI_MATCH(ident) \
    ((strcmp(interface, z##ident##_interface.name) == 0) && (platform->ident == NULL))

    if WSI_MATCH(wl_compositor) {
        platform->wl_compositor = wsi_global_bind(
            platform,
            name,
            &wl_compositor_interface,
            NULL,
            version,
            WSI_WL_COMPOSITOR_VERSION);
    }
    else if WSI_MATCH(wl_shm) {
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
    else if WSI_MATCH(wp_viewporter) {
        platform->wp_viewporter = wsi_global_bind(
            platform,
            name,
            &wp_viewporter_interface,
            NULL,
            version,
            WSI_WP_VIEWPORTER_VERSION);
    }
    else if WSI_MATCH(wp_fractional_scale_manager_v1) {
        platform->wp_fractional_scale_manager_v1 = wsi_global_bind(
            platform,
            name,
            &wp_fractional_scale_manager_v1_interface,
            NULL,
            version,
            WSI_WP_FRACTIONAL_SCALE_MANAGER_V1_VERSION);
    }
    else if ZWSI_MATCH(wp_input_timestamps_manager_v1) {
        platform->wp_input_timestamps_manager_v1 = wsi_global_bind(
            platform,
            name,
            &zwp_input_timestamps_manager_v1_interface,
            NULL,
            version,
            WSI_WP_INPUT_TIMESTAMPS_MANAGER_V1_VERSION);
    }
    else if ZWSI_MATCH(wp_relative_pointer_manager_v1) {
        platform->wp_relative_pointer_manager_v1 = wsi_global_bind(
            platform,
            name,
            &zwp_relative_pointer_manager_v1_interface,
            NULL,
            version,
            WSI_WP_RELATIVE_POINTER_MANAGER_V1_VERSION);
    }
    else if ZWSI_MATCH(wp_pointer_constraints_v1) {
        platform->wp_pointer_constraints_v1 = wsi_global_bind(
            platform,
            name,
            &zwp_pointer_constraints_v1_interface,
            NULL,
            version,
            WSI_WP_POINTER_CONSTRAINTS_V1_VERSION);
    }
    else if ZWSI_MATCH(wp_pointer_gestures_v1) {
        platform->wp_pointer_gestures_v1 = wsi_global_bind(
            platform,
            name,
            &zwp_pointer_gestures_v1_interface,
            NULL,
            version,
            WSI_WP_POINTER_GESTURES_V1_VERSION);
    }
    else if ZWSI_MATCH(wp_keyboard_shortcuts_inhibit_manager_v1) {
        platform->wp_keyboard_shortcuts_inhibit_manager_v1 = wsi_global_bind(
            platform,
            name,
            &zwp_keyboard_shortcuts_inhibit_manager_v1_interface,
            NULL,
            version,
            WSI_WP_KEYBOARD_SHORTCUTS_INHIBIT_MANAGER_V1_VERSION);
    }
    else if ZWSI_MATCH(wp_idle_inhibit_manager_v1) {
        platform->wp_idle_inhibit_manager_v1 = wsi_global_bind(
            platform,
            name,
            &zwp_idle_inhibit_manager_v1_interface,
            NULL,
            version,
            WSI_WP_IDLE_INHIBIT_MANAGER_V1_VERSION);
    }
    else if WSI_MATCH(wp_content_type_manager_v1) {
        platform->wp_content_type_manager_v1 = wsi_global_bind(
            platform,
            name,
            &wp_content_type_manager_v1_interface,
            NULL,
            version,
            WSI_WP_CONTENT_TYPE_MANAGER_V1_VERSION);
    }
    else if WSI_MATCH(xdg_wm_base) {
        platform->xdg_wm_base = wsi_global_bind(
            platform,
            name,
            &xdg_wm_base_interface,
            &xdg_wm_base_listener,
            version,
            WSI_XDG_WM_BASE_VERSION);
    }
    else if ZWSI_MATCH(xdg_decoration_manager_v1) {
        platform->xdg_decoration_manager_v1 = wsi_global_bind(
            platform,
            name,
            &zxdg_decoration_manager_v1_interface,
            NULL,
            version,
            WSI_XDG_DECORATION_MANAGER_V1_VERSION);
    }
    else if ZWSI_MATCH(xdg_output_manager_v1) {
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
    else if WSI_MATCH(ext_idle_notifier_v1) {
        platform->ext_idle_notifier_v1 = wsi_global_bind(
            platform,
            name,
            &ext_idle_notifier_v1_interface,
            NULL,
            version,
            WSI_EXT_IDLE_NOTIFICATION_V1_VERSION);
    }

#undef WSI_MATCH
#undef ZWSI_MATCH
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
        if (seat->global.name == name) {
            wsi_seat_remove(seat);
            return;
        }
    }

    struct wsi_output *output, *output_tmp;
    wl_list_for_each_safe(output, output_tmp, &platform->output_list, link) {
        if (output->global.name == name) {
            wsi_output_destroy(output);
            return;
        }
    }

#define WSI_GLOBAL_REMOVE(ident) \
    if (platform->ident != NULL) { \
        struct wsi_global *global = ident##_get_user_data(platform->ident); \
        if (global->name == name) { \
            ident##_destroy(platform->ident); \
            platform->ident = NULL;  \
            wsi_global_destroy(global); \
            return; \
        } \
    }

#define ZWSI_GLOBAL_REMOVE(ident) \
    if (platform->ident != NULL) { \
        struct wsi_global *global = z##ident##_get_user_data(platform->ident); \
        if (global->name == name) { \
            z##ident##_destroy(platform->ident); \
            platform->ident = NULL;  \
            wsi_global_destroy(global); \
            return; \
        } \
    }

    WSI_GLOBAL_REMOVE(wp_viewporter)
    WSI_GLOBAL_REMOVE(wp_fractional_scale_manager_v1)
    WSI_GLOBAL_REMOVE(wp_content_type_manager_v1)
    WSI_GLOBAL_REMOVE(ext_idle_notifier_v1)

    ZWSI_GLOBAL_REMOVE(wp_input_timestamps_manager_v1)
    ZWSI_GLOBAL_REMOVE(wp_relative_pointer_manager_v1)
    ZWSI_GLOBAL_REMOVE(wp_pointer_constraints_v1)
    ZWSI_GLOBAL_REMOVE(wp_keyboard_shortcuts_inhibit_manager_v1)
    ZWSI_GLOBAL_REMOVE(wp_idle_inhibit_manager_v1)
    ZWSI_GLOBAL_REMOVE(xdg_decoration_manager_v1)
    ZWSI_GLOBAL_REMOVE(xdg_output_manager_v1)

#undef WSI_GLOBAL_REMOVE
#undef ZWSI_GLOBAL_REMOVE

    if (platform->wp_pointer_gestures_v1) {
        struct wsi_global *global = zwp_pointer_gestures_v1_get_user_data(
            platform->wp_pointer_gestures_v1);
        if (global->name == name) {
            if (zwp_pointer_gestures_v1_get_version(platform->wp_pointer_gestures_v1) >=
                ZWP_POINTER_GESTURES_V1_RELEASE_SINCE_VERSION)
            {
                zwp_pointer_gestures_v1_release(platform->wp_pointer_gestures_v1);
            } else {
                zwp_pointer_gestures_v1_destroy(platform->wp_pointer_gestures_v1);
            }
            platform->wp_pointer_gestures_v1 = NULL;
            wsi_global_destroy(global);
        }
    }

    // TODO: Remove temporary aborts and implement proper platform invalidation

    if (platform->xdg_wm_base != NULL) {
        struct wsi_global *global = xdg_wm_base_get_user_data(platform->xdg_wm_base);
        if (global->name == name) {
            abort();
        }
    }

    if (platform->wl_compositor != NULL) {
        struct wsi_global *global = wl_compositor_get_user_data(platform->wl_compositor);
        if (global->name == name) {
            abort();
        }
    }

    if (platform->wl_shm != NULL) {
        struct wsi_global *global = wl_shm_get_user_data(platform->wl_shm);
        if (global->name == name) {
            abort();
        }
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

    if (!platform->wl_compositor ||
        !platform->wl_shm ||
        !platform->xdg_wm_base)
    {
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
