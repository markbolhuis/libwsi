#include <memory.h>
#include <assert.h>

#include <wayland-util.h>

#include "common_priv.h"
#include "platform_priv.h"
#include "window_priv.h"

#include "wsi/vulkan.h"
#include <vulkan/vulkan_wayland.h>

#define array_size(arr) (sizeof((arr)) / sizeof(arr[0]))

const char *INSTANCE_EXTS[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
};

const char *DEVICE_EXTS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static WsiResult
wsi_fill_array(uint32_t src_c, const char *const *src, uint32_t *dst_c, const char **dst)
{
    if (dst == NULL) {
        *dst_c = src_c;
        return WSI_SUCCESS;
    }

    WsiResult res = WSI_SUCCESS;
    if (src_c > *dst_c) {
        src_c = *dst_c;
        res = WSI_INCOMPLETE;
    }

    memcpy(dst, src, src_c * sizeof(char *));
    *dst_c = src_c;
    return res;
}

WsiResult
wsiEnumerateRequiredInstanceExtensions(
    __attribute__((unused)) WsiPlatform platform,
    uint32_t *pExtensionCount,
    const char **ppExtensions)
{
    return wsi_fill_array(array_size(INSTANCE_EXTS), INSTANCE_EXTS, pExtensionCount, ppExtensions);
}

WsiResult
wsiEnumerateRequiredDeviceExtensions(
    __attribute__((unused)) WsiPlatform platform,
    uint32_t *pExtensionCount,
    const char **ppExtensions)
{
    return wsi_fill_array(array_size(DEVICE_EXTS), DEVICE_EXTS, pExtensionCount, ppExtensions);
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
    WsiWindow window,
    VkInstance instance,
    const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface)
{
    if (window->api != WSI_API_NONE) {
        return WSI_ERROR_WINDOW_IN_USE;
    }

    VkWaylandSurfaceCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .display = window->platform->wl_display,
        .surface = window->wl_surface,
    };

    VkResult res = vkCreateWaylandSurfaceKHR(instance, &info, pAllocator, pSurface);
    return res == VK_SUCCESS ? WSI_SUCCESS : WSI_ERROR_VULKAN;
}
