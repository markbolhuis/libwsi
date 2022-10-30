#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>

#include "wsi/platform.h"
#include "wsi/window.h"

#include "utils.h"

#include "platform_priv.h"
#include "window_priv.h"

static bool
wsi_query_extension(struct wsi_platform *platform, const char *name)
{
    xcb_query_extension_cookie_t cookie = xcb_query_extension(
        platform->xcb_connection,
        strlen(name),
        name);

    xcb_query_extension_reply_t *reply = xcb_query_extension_reply(
        platform->xcb_connection,
        cookie,
        NULL);

    bool present = false;
    if (reply) {
        present = reply->present;
        free(reply);
    }

    return present;
}

static void
wsi_init_atoms(struct wsi_platform *platform)
{
    struct {
        const char *name;
        xcb_atom_t *atom;
    } table[2] = {
        { "WM_PROTOCOLS", &platform->xcb_atom_wm_protocols },
        { "WM_DELETE_WINDOW", &platform->xcb_atom_wm_delete_window },
    };

    const int count = wsi_array_length(table);
    xcb_intern_atom_cookie_t cookies[count];

    for (size_t i = 0; i < count; ++i) {
        cookies[i] = xcb_intern_atom(
            platform->xcb_connection, 0, strlen(table[i].name), table[i].name);
    }

    for (size_t i = 0; i < count; ++i) {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(
            platform->xcb_connection, cookies[i], NULL);

        if (reply) {
            *table[i].atom = reply->atom;
            free(reply);
        } else {
            *table[i].atom = XCB_ATOM_NONE;
        }
    }
}

static xcb_screen_t *
wsi_xcb_get_screen(const xcb_setup_t *setup, int screen)
{
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    for (; iter.rem; --screen, xcb_screen_next(&iter))
    {
        if (screen == 0) {
            return iter.data;
        }
    }

    return NULL;
}

static struct wsi_window *
wsi_find_window(struct wsi_platform *platform, xcb_window_t window)
{
    struct wsi_window *wsi_window = NULL;
    wsi_list_for_each(wsi_window, &platform->window_list, link) {
        if (wsi_window->xcb_window == window) {
            return wsi_window;
        }
    }

    return NULL;
}

static enum wsi_result
wsi_platform_init(struct wsi_platform *platform)
{
    enum wsi_result result;

    wsi_list_init(&platform->window_list);

    platform->xcb_connection = xcb_connect(NULL, &platform->xcb_screen_id);
    int err = xcb_connection_has_error(platform->xcb_connection);
    if (err > 0) {
        result = WSI_ERROR_PLATFORM;
        goto err_connect;
    }

    platform->default_queue.platform = platform;

    const xcb_setup_t *setup = xcb_get_setup(platform->xcb_connection);

    platform->xcb_screen = wsi_xcb_get_screen(setup, platform->xcb_screen_id);
    if (!platform->xcb_screen) {
        result = WSI_ERROR_PLATFORM;
        goto err_screen;
    }

    if (!wsi_query_extension(platform, "RANDR")) {
        result = WSI_ERROR_PLATFORM;
        goto err_extension;
    }

    if (!wsi_query_extension(platform, "XInputExtension")) {
        result = WSI_ERROR_PLATFORM;
        goto err_extension;
    }

    wsi_init_atoms(platform);

    return WSI_SUCCESS;

err_extension:
err_screen:
err_connect:
    xcb_disconnect(platform->xcb_connection);
    return result;
}

static void
wsi_platform_uninit(struct wsi_platform *platform)
{
    xcb_disconnect(platform->xcb_connection);
}

WsiResult
wsiCreatePlatform(WsiPlatform *pPlatform)
{
    struct wsi_platform *p = calloc(1, sizeof(struct wsi_platform));
    if (!p) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    enum wsi_result result = wsi_platform_init(p);
    if (result != WSI_SUCCESS) {
        free(p);
        return result;
    }

    *pPlatform = p;
    return WSI_SUCCESS;
}

void
wsiDestroyPlatform(WsiPlatform platform)
{
    wsi_platform_uninit(platform);
    free(platform);
}

WsiEventQueue
wsiGetDefaultEventQueue(WsiPlatform platform)
{
    return &platform->default_queue;
}

WsiResult
wsiCreateEventQueue(WsiPlatform platform, WsiEventQueue *pEventQueue)
{
    // TODO: Unimplemented
    abort();
}

void
wsiDestroyEventQueue(WsiEventQueue eventQueue)
{
    // TODO: Unimplemented
    abort();
}

WsiResult
wsiDispatchEvents(WsiEventQueue eventQueue, int64_t timeout)
{
    struct wsi_platform *platform = eventQueue->platform;
    while(true) {
        xcb_generic_event_t *event = xcb_poll_for_event(platform->xcb_connection);
        if (!event) {
            break;
        }

        switch (event->response_type & ~0x80) {
            case XCB_CONFIGURE_NOTIFY: {
                xcb_configure_notify_event_t *notify
                    = (xcb_configure_notify_event_t *)event;

                struct wsi_window *window
                    = wsi_find_window(platform, notify->window);
                if (window) {
                    wsi_window_xcb_configure_notify(window, notify);
                }
                break;
            }
            case XCB_CLIENT_MESSAGE: {
                xcb_client_message_event_t *message
                    = (xcb_client_message_event_t *)event;

                struct wsi_window *window
                    = wsi_find_window(platform, message->window);
                if (window) {
                    wsi_window_xcb_client_message(window, message);
                }
                break;
            }
        }

        free(event);
    }

    return WSI_SUCCESS;
}
