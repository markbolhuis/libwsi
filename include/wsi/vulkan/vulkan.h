#ifndef WSI_INCLUDE_VULKAN_VULKAN_H
#define WSI_INCLUDE_VULKAN_VULKAN_H

#include <vulkan/vulkan_core.h>

#include "../common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef WsiResult (*PFN_wsiEnumerateRequiredInstanceExtensions)(WsiPlatform platform, uint32_t *pExtensionCount, const char **ppExtensions);
typedef WsiResult (*PFN_wsiEnumerateRequiredDeviceExtensions)(WsiPlatform platform, uint32_t *pExtensionCount, const char **ppExtensions);
typedef WsiResult (*PFN_wsiCreateWindowSurface)(WsiPlatform platform, WsiWindow window, VkInstance instance, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface);
typedef VkBool32 (*PFN_wsiGetPhysicalDevicePresentationSupport)(WsiPlatform platform, VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex);

WsiResult
wsiEnumerateRequiredInstanceExtensions(
    WsiPlatform platform,
    uint32_t *pExtensionCount,
    const char **ppExtensions);

WsiResult
wsiEnumerateRequiredDeviceExtensions(
    WsiPlatform platform,
    uint32_t *pExtensionCount,
    const char **ppExtensions);

WsiResult
wsiCreateWindowSurface(
    WsiPlatform platform,
    WsiWindow window,
    VkInstance instance,
    const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface);

VkBool32
wsiGetPhysicalDevicePresentationSupport(
    WsiPlatform platform,
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex);

#ifdef __cplusplus
}
#endif

#endif
