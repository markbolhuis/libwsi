#include <stdlib.h>

#include <xcb/xcb.h>

#include "wsi/platform.h"

#include "platform_priv.h"

WsiResult
wsiCreatePlatform(WsiPlatform *pPlatform)
{
    struct wsi_platform *platform = calloc(1, sizeof(struct wsi_platform));
    if (!platform) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    int screen = 0;
    platform->xcb_connection = xcb_connect(NULL, &screen);
    int err = xcb_connection_has_error(platform->xcb_connection);
    if (err > 0) {
        free(platform);
        return WSI_ERROR_PLATFORM;
    }

    platform->xcb_screen_id = screen;

    const xcb_setup_t *setup = xcb_get_setup(platform->xcb_connection);
    for (xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
         iter.rem;
         --screen, xcb_screen_next (&iter))
    {
        if (screen == 0) {
            platform->xcb_screen = iter.data;
            break;
        }
    }

    *pPlatform = platform;
    return WSI_SUCCESS;
}

void
wsiDestroyPlatform(WsiPlatform platform)
{
    xcb_disconnect(platform->xcb_connection);
    free(platform);
}

void
wsiPoll(WsiPlatform platform)
{
    xcb_generic_event_t *event = xcb_poll_for_event(platform->xcb_connection);
    free(event);
}
