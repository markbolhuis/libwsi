#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>

#include "wsi/platform.h"
#include "wsi/window.h"

#include "utils.h"

#include "platform_priv.h"
#include "window_priv.h"

void
wsi_get_xcb_atoms(
    struct wsi_platform *platform,
    const char *const *names,
    size_t count,
    xcb_atom_t *atoms)
{
    xcb_connection_t *connection = platform->xcb_connection;

    xcb_intern_atom_cookie_t *cookies
        = calloc(count, sizeof(xcb_intern_atom_cookie_t));

    for (size_t i = 0; i < count; ++i) {
        cookies[i] = xcb_intern_atom(connection, 0, strlen(names[i]), names[i]);
    }

    for (size_t i = 0; i < count; ++i) {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(
            connection, cookies[i], NULL);

        if (reply) {
            atoms[i] = reply->atom;
            free(reply);
        } else {
            atoms[i] = XCB_ATOM_NONE;
        }
    }

    free(cookies);
}

static xcb_screen_t *
wsi_xcb_get_screen(
    const xcb_setup_t *setup,
    int screen)
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

WsiResult
wsiCreatePlatform(WsiPlatform *pPlatform)
{
    enum wsi_result result;

    const char *const atom_names[] = {
        "WM_PROTOCOLS",
        "WM_DELETE_WINDOW",
    };

    const int atom_count = wsi_array_length(atom_names);
    xcb_atom_t atoms[atom_count];

    struct wsi_platform *platform = calloc(1, sizeof(struct wsi_platform));
    if (!platform) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

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

    wsi_get_xcb_atoms(platform, atom_names, atom_count, atoms);
    platform->xcb_atom_wm_protocols = atoms[0];
    platform->xcb_atom_wm_delete_window = atoms[1];

    *pPlatform = platform;
    return WSI_SUCCESS;

err_screen:
err_connect:
    xcb_disconnect(platform->xcb_connection);
    free(platform);
    return result;
}

void
wsiDestroyPlatform(WsiPlatform platform)
{
    xcb_disconnect(platform->xcb_connection);
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
wsiPollEventQueue(WsiEventQueue eventQueue)
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

                struct wsi_window *window;
                wsi_list_for_each(window, &platform->window_list, link) {
                    if (notify->window == window->xcb_window) {
                        wsi_window_xcb_configure_notify(window, notify);
                    }
                }

                break;
            }
            case XCB_CLIENT_MESSAGE: {
                xcb_client_message_event_t *message
                    = (xcb_client_message_event_t *)event;

                struct wsi_window *window;
                wsi_list_for_each(window, &platform->window_list, link) {
                    if (message->window == window->xcb_window) {
                        wsi_window_xcb_client_message(window, message);
                    }
                }

                break;
            }
        }

        free(event);
    }

    return WSI_SUCCESS;
}
