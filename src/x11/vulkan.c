#include <assert.h>
#include <memory.h>

#include <xcb/xcb.h>

#include "utils.h"

#include "common_priv.h"
#include "platform_priv.h"
#include "window_priv.h"

#include "wsi/vulkan.h"
#include <vulkan/vulkan_xcb.h>

const uint32_t INSTANCE_EXTENSION_COUNT = 2;
const char *INSTANCE_EXTENSION_NAMES[2] ={
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
};

const uint32_t DEVICE_EXTENSION_COUNT = 1;
const char *DEVICE_EXTENSION_NAMES[1] ={
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

WsiResult
wsiEnumerateRequiredInstanceExtensions(
    __attribute__((unused)) WsiPlatform platform,
    uint32_t *pExtensionCount,
    const char **ppExtensions)
{
    if (ppExtensions == NULL) {
        *pExtensionCount = INSTANCE_EXTENSION_COUNT;
        return WSI_SUCCESS;
    }

    WsiResult res = WSI_SUCCESS;
    uint32_t cpy = INSTANCE_EXTENSION_COUNT;
    if (cpy > *pExtensionCount) {
        cpy = *pExtensionCount;
        res = WSI_INCOMPLETE;
    }

    memcpy(ppExtensions, INSTANCE_EXTENSION_NAMES, sizeof(char *) * cpy);
    *pExtensionCount = cpy;
    return res;
}

WsiResult
wsiEnumerateRequiredDeviceExtensions(
    __attribute__((unused)) WsiPlatform platform,
    uint32_t *pExtensionCount,
    const char **ppExtensions)
{
    if (ppExtensions == NULL) {
        *pExtensionCount = DEVICE_EXTENSION_COUNT;
        return WSI_SUCCESS;
    }

    WsiResult res = WSI_SUCCESS;
    uint32_t cpy = DEVICE_EXTENSION_COUNT;
    if (cpy > *pExtensionCount) {
        cpy = *pExtensionCount;
        res = WSI_INCOMPLETE;
    }

    memcpy(ppExtensions, DEVICE_EXTENSION_NAMES, sizeof(char *) * cpy);
    *pExtensionCount = cpy;
    return res;
}

VkBool32
wsiGetPhysicalDevicePresentationSupport(
    WsiPlatform platform,
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex)
{
    return vkGetPhysicalDeviceXcbPresentationSupportKHR(
        physicalDevice,
        queueFamilyIndex,
        platform->xcb_connection,
        platform->xcb_screen->root_visual);
}

WsiResult
wsiCreateWindowSurface(
    WsiWindow window,
    VkInstance instance,
    const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface)
{
    struct wsi_platform *platform = window->platform;

    if (window->api != WSI_API_NONE) {
        return WSI_ERROR_WINDOW_IN_USE;
    }

    VkXcbSurfaceCreateInfoKHR xcbInfo = {0};
    xcbInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    xcbInfo.pNext = NULL;
    xcbInfo.flags = 0;
    xcbInfo.connection = platform->xcb_connection;
    xcbInfo.window = window->xcb_window;

    VkSurfaceKHR surface;
    VkResult vres = vkCreateXcbSurfaceKHR(
        instance,
        &xcbInfo,
        pAllocator,
        &surface);

    WsiResult res;
    if (vres == VK_SUCCESS) {
        res = WSI_SUCCESS;
        window->api = WSI_API_VULKAN;
        *pSurface = surface;
    } else {
        res = WSI_ERROR_VULKAN;
    }

    return res;
}
