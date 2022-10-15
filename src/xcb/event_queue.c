#include <stdlib.h>

#include <xcb/xcb.h>

#include "wsi/window.h"
#include "wsi/event_queue.h"

#include "utils.h"

#include "common_priv.h"
#include "platform_priv.h"
#include "event_queue_priv.h"
#include "window_priv.h"

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
}
