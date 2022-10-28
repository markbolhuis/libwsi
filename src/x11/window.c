#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <xcb/xcb.h>

#include "wsi/window.h"

#include "utils.h"

#include "common_priv.h"
#include "platform_priv.h"
#include "window_priv.h"

// region XCB Events

void
wsi_window_xcb_configure_notify(
    struct wsi_window *window,
    const xcb_configure_notify_event_t *event)
{
    assert(event->window == window->xcb_window);

    struct wsi_extent extent = {
        .width = event->width,
        .height = event->height
    };

    window->pfn_configure(window->user_data, extent);
}

void
wsi_window_xcb_client_message(
    struct wsi_window *window,
    const xcb_client_message_event_t *event)
{
    assert(event->window == window->xcb_window);

    if (event->type == window->platform->xcb_atom_wm_protocols &&
        event->data.data32[0] == window->platform->xcb_atom_wm_delete_window)
    {
        window->pfn_close(window->user_data);
    }
}

// endregion


WsiResult
wsiCreateWindow(
    WsiPlatform platform,
    const WsiWindowCreateInfo *pCreateInfo,
    WsiWindow *pWindow)
{
    struct wsi_window *window = calloc(1, sizeof(struct wsi_window));
    if (!window) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    window->platform = platform;
    window->api = WSI_API_NONE;
    window->xcb_window = xcb_generate_id(platform->xcb_connection);
    window->user_width = wsi_xcb_clamp(pCreateInfo->extent.width);
    window->user_height = wsi_xcb_clamp(pCreateInfo->extent.height);
    window->pfn_close = pCreateInfo->pfnClose;
    window->pfn_configure = pCreateInfo->pfnConfigure;
    window->user_data = pCreateInfo->pUserData;

    if (pCreateInfo->parent) {
        window->xcb_parent = pCreateInfo->parent->xcb_window;
    } else {
        window->xcb_parent = platform->xcb_screen->root;
    }

    uint32_t value_list[2];
    uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK ;
    value_list[0] = platform->xcb_screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_EXPOSURE
               // | XCB_EVENT_MASK_RESIZE_REDIRECT
                  | XCB_EVENT_MASK_STRUCTURE_NOTIFY
                  | XCB_EVENT_MASK_BUTTON_PRESS
                  | XCB_EVENT_MASK_BUTTON_RELEASE;

    xcb_create_window_checked(
        platform->xcb_connection,
        XCB_COPY_FROM_PARENT,
        window->xcb_window,
        window->xcb_parent,
        0, 0,
        window->user_width,
        window->user_height,
        10,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        XCB_COPY_FROM_PARENT,
        value_mask,
        value_list);

    xcb_atom_t properties[] = {
        platform->xcb_atom_wm_protocols,
        platform->xcb_atom_wm_delete_window,
    };

    xcb_change_property(
        platform->xcb_connection,
        XCB_PROP_MODE_REPLACE,
        window->xcb_window,
        platform->xcb_atom_wm_protocols,
        XCB_ATOM_ATOM,
        32,
        wsi_array_length(properties),
        properties);

    xcb_map_window(platform->xcb_connection, window->xcb_window);

    wsi_list_insert(&platform->window_list, &window->link);
    *pWindow = window;
    return WSI_SUCCESS;
}

void
wsiDestroyWindow(WsiWindow window)
{
    struct wsi_platform *platform = window->platform;

    xcb_unmap_window(platform->xcb_connection, window->xcb_window);
    xcb_destroy_window(platform->xcb_connection, window->xcb_window);
    xcb_flush(platform->xcb_connection);

    free(window);
}

WsiResult
wsiSetWindowParent(WsiWindow window, WsiWindow parent)
{
    struct wsi_platform *platform = window->platform;
    if (parent) {
        window->xcb_parent = parent->xcb_window;
    } else {
        window->xcb_parent = platform->xcb_screen->root;
    }
    xcb_reparent_window(
        platform->xcb_connection,
        window->xcb_window,
        parent->xcb_window,
        0, 0);
    return WSI_SUCCESS;
}

WsiResult
wsiSetWindowTitle(WsiWindow window, const char *pTitle)
{
    if (pTitle) {
        xcb_change_property(
            window->platform->xcb_connection,
            XCB_PROP_MODE_REPLACE,
            window->xcb_window,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8,
            strlen(pTitle),
            pTitle);
    } else {
        xcb_delete_property(
           window->platform->xcb_connection,
           window->xcb_window,
           XCB_ATOM_WM_NAME);
    }

    return WSI_SUCCESS;
}
