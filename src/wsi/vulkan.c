#include "wsi/vulkan.h"

#include "platform_priv.h"
#include "window_priv.h"


WsiResult
wsiEnumerateRequiredInstanceExtensions(
    WsiPlatform platform,
    uint32_t *pExtensionCount,
    const char **ppExtensions)
{
    PFN_wsiEnumerateRequiredInstanceExtensions sym = wsi_platform_dlsym(platform, "wsiEnumerateRequiredInstanceExtensions");
    return sym(platform, pExtensionCount, ppExtensions);
}

WsiResult
wsiEnumerateRequiredDeviceExtensions(
    WsiPlatform platform,
    uint32_t *pExtensionCount,
    const char **ppExtensions)
{
    PFN_wsiEnumerateRequiredDeviceExtensions sym = wsi_platform_dlsym(platform, "wsiEnumerateRequiredDeviceExtensions");
    return sym(platform, pExtensionCount, ppExtensions);
}

VkBool32
wsiGetPhysicalDevicePresentationSupport(
    WsiPlatform platform,
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex)
{
    PFN_wsiGetPhysicalDevicePresentationSupport sym = wsi_platform_dlsym(platform, "wsiGetPhysicalDevicePresentationSupport");
    return sym(platform->platform, physicalDevice, queueFamilyIndex);
}

WsiResult
wsiCreateWindowSurface(
    WsiWindow window,
    VkInstance instance,
    const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface)
{
    PFN_wsiCreateWindowSurface sym = wsi_window_dlsym(window, "wsiCreateWindowSurface");
    return sym(window->window, instance, pAllocator, pSurface);
}
