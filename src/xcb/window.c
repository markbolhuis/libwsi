#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <xcb/xcb.h>

#include "wsi/window.h"

#include "platform_priv.h"
#include "window_priv.h"

static inline struct wsi_xcb_extent
wsi_extent_to_xcb(
    struct wsi_extent extent)
{
    assert(extent.width <= UINT16_MAX);
    assert(extent.height <= UINT16_MAX);

    struct wsi_xcb_extent xcb = {
        .width = (uint16_t)extent.width,
        .height = (uint16_t)extent.height,
    };

    return xcb;
}

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
    window->xcb_window = xcb_generate_id(platform->xcb_connection);

    window->extent = wsi_extent_to_xcb(pCreateInfo->extent);

    uint32_t value_list[2];
    uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK ;
    value_list[0] = platform->xcb_screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_EXPOSURE
                  | XCB_EVENT_MASK_RESIZE_REDIRECT
                  | XCB_EVENT_MASK_BUTTON_PRESS
                  | XCB_EVENT_MASK_BUTTON_RELEASE;

    xcb_create_window_checked(
        platform->xcb_connection,
        XCB_COPY_FROM_PARENT,
        window->xcb_window,
        platform->xcb_screen->root,
        0, 0,
        window->extent.width,
        window->extent.height,
        10,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        platform->xcb_screen->root_visual,
        value_mask,
        value_list);

    xcb_map_window(platform->xcb_connection, window->xcb_window);

    *pWindow = window;
    return WSI_SUCCESS;
}

void
wsiDestroyWindow(
    WsiPlatform platform,
    WsiWindow window)
{
    xcb_unmap_window(platform->xcb_connection, window->xcb_window);
    xcb_destroy_window(platform->xcb_connection, window->xcb_window);

    free(window);
}

void
wsiGetWindowFeatures(
    WsiWindow window,
    WsiWindowFeatures *pFeatures)
{

}

WsiResult
wsiGetWindowParent(
    WsiWindow window,
    WsiWindow *pParent)
{
    return WSI_ERROR_NOT_IMPLEMENTED;
}

WsiResult
wsiSetWindowParent(
    WsiWindow window,
    WsiWindow parent)
{
    return WSI_ERROR_NOT_IMPLEMENTED;
}

void
wsiGetWindowExtent(
    WsiWindow window,
    WsiExtent *pExtent)
{
    pExtent->width = window->extent.width;
    pExtent->height = window->extent.height;
}

WsiResult
wsiSetWindowTitle(
    WsiWindow window,
    const char *pTitle)
{
    xcb_change_property(
        window->platform->xcb_connection,
        XCB_PROP_MODE_REPLACE,
        window->xcb_window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        strlen(pTitle),
        pTitle);

    return WSI_SUCCESS;
}
