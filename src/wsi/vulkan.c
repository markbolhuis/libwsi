#include <dlfcn.h>

#include "wsi/vulkan.h"

extern void *g_handle;

WsiResult
wsiEnumerateRequiredInstanceExtensions(
    WsiPlatform platform,
    uint32_t *pExtensionCount,
    const char **ppExtensions)
{
    PFN_wsiEnumerateRequiredInstanceExtensions sym
        = (PFN_wsiEnumerateRequiredInstanceExtensions)
            dlsym(g_handle, "wsiEnumerateRequiredInstanceExtensions");
    return sym(platform, pExtensionCount, ppExtensions);
}

WsiResult
wsiEnumerateRequiredDeviceExtensions(
    WsiPlatform platform,
    uint32_t *pExtensionCount,
    const char **ppExtensions)
{
    PFN_wsiEnumerateRequiredDeviceExtensions sym
        = (PFN_wsiEnumerateRequiredDeviceExtensions)
            dlsym(g_handle, "wsiEnumerateRequiredDeviceExtensions");
    return sym(platform, pExtensionCount, ppExtensions);
}

VkBool32
wsiGetPhysicalDevicePresentationSupport(
    WsiPlatform platform,
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex)
{
    PFN_wsiGetPhysicalDevicePresentationSupport sym
        = (PFN_wsiGetPhysicalDevicePresentationSupport)
            dlsym(g_handle, "wsiGetPhysicalDevicePresentationSupport");
    return sym(platform, physicalDevice, queueFamilyIndex);
}

WsiResult
wsiCreateWindowSurface(
    WsiWindow window,
    VkInstance instance,
    const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface)
{
    PFN_wsiCreateWindowSurface sym
        = (PFN_wsiCreateWindowSurface)
            dlsym(g_handle, "wsiCreateWindowSurface");
    return sym(window, instance, pAllocator, pSurface);
}
