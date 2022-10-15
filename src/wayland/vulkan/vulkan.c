#include <memory.h>
#include <assert.h>

#include <wayland-util.h>

#include "wsi/window.h"
#include "wsi/vulkan/vulkan.h"

#include <vulkan/vulkan_wayland.h>

#include "../common_priv.h"
#include "../platform_priv.h"
#include "../window_priv.h"

const uint32_t INSTANCE_EXTENSION_COUNT = 2;
const char *INSTANCE_EXTENSION_NAMES[2] ={
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
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

    enum wsi_result res = WSI_SUCCESS;
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

    enum wsi_result res = WSI_SUCCESS;
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
    return vkGetPhysicalDeviceWaylandPresentationSupportKHR(
        physicalDevice,
        queueFamilyIndex,
        platform->wl_display);
}

WsiResult
wsiCreateWindowSurface(
    WsiPlatform platform,
    WsiWindow window,
    VkInstance instance,
    const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface)
{
    assert(window->platform == platform);

    if (window->api != WSI_API_NONE) {
        return WSI_ERROR_WINDOW_IN_USE;
    }

    VkWaylandSurfaceCreateInfoKHR wlInfo = {0};
    wlInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    wlInfo.pNext = NULL;
    wlInfo.flags = 0;
    wlInfo.display = platform->wl_display;
    wlInfo.surface = window->wl_surface;

    VkSurfaceKHR surface;
    VkResult vres = vkCreateWaylandSurfaceKHR(
        instance,
        &wlInfo,
        pAllocator,
        &surface);

    enum wsi_result res;
    if (vres == VK_SUCCESS) {
        res = WSI_SUCCESS;
        window->api = WSI_API_VULKAN;
        *pSurface = surface;
    } else {
        res = WSI_ERROR_VULKAN;
    }

    return res;
}
