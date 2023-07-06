#include <assert.h>
#include <memory.h>

#include "common_priv.h"
#include "platform_priv.h"
#include "window_priv.h"

#include "wsi/vulkan.h"

#include <vulkan/vulkan_metal.h>

#import <QuartzCore/QuartzCore.h>

#define array_size(arr) (sizeof((arr)) / sizeof(arr[0]))

const char *INSTANCE_EXTS[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_EXT_METAL_SURFACE_EXTENSION_NAME,
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
};

const char *DEVICE_EXTS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset",
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
    //TODO
    return VK_TRUE;
}

WsiResult
wsiCreateWindowSurface(
    WsiWindow window,
    VkInstance instance,
    const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface)
{
    //if (window->api != WSI_API_NONE) {
    //    return WSI_ERROR_WINDOW_IN_USE;
    //}

    CAMetalLayer* layer = [CAMetalLayer layer];
    //[layer setContentsScale:window->framebufferScaleX];
    window->layer = layer;

    NSWindow* nswindow = window->window;
    nswindow.contentView.layer = layer;
    nswindow.contentView.wantsLayer = YES;

    VkMetalSurfaceCreateInfoEXT surfaceCreateInfo;
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    surfaceCreateInfo.pNext = NULL;
    surfaceCreateInfo.pLayer = layer;
    surfaceCreateInfo.flags = 0;

    VkResult res = vkCreateMetalSurfaceEXT(instance, &surfaceCreateInfo, pAllocator, pSurface);

    return res == VK_SUCCESS ? WSI_SUCCESS : WSI_ERROR_VULKAN;
}
