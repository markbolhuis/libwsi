#include "wsi/vulkan/vulkan.h"

#include "platform_priv.h"
#include "window_priv.h"

typedef WsiResult (*PFN_wsiEnumerateRequiredInstanceExtensions)(WsiPlatform platform, uint32_t *pExtensionCount, const char **ppExtensions);
typedef WsiResult (*PFN_wsiEnumerateRequiredDeviceExtensions)(WsiPlatform platform, uint32_t *pExtensionCount, const char **ppExtensions);
typedef WsiResult (*PFN_wsiCreateWindowSurface)(WsiPlatform platform, WsiWindow window, VkInstance instance, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface);
typedef VkBool32 (*PFN_wsiGetPhysicalDevicePresentationSupport)(WsiPlatform platform, VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex);

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
    WsiPlatform platform,
    WsiWindow window,
    VkInstance instance,
    const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface)
{
    PFN_wsiCreateWindowSurface sym = wsi_platform_dlsym(platform, "wsiCreateWindowSurface");
    return sym(platform->platform, window->window, instance, pAllocator, pSurface);
}
