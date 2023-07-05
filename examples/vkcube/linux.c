#define _POSIX_C_SOURCE 201112L
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <time.h>

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <vulkan/vulkan_core.h>

VkResult
demo_create_shader_module(VkDevice device, const char *path, VkShaderModule *module)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return VK_ERROR_UNKNOWN;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return VK_ERROR_UNKNOWN;
    }

    void *data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        printf("Failed to open file '%s'", path);
        close(fd);
        return VK_ERROR_UNKNOWN;
    }

    VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = st.st_size,
        .pCode = data,
    };

    VkResult res = vkCreateShaderModule(device, &info, NULL, module);
    munmap(data, st.st_size);
    close(fd);
    return res;
}

int64_t
demo_get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000 + (int64_t)ts.tv_nsec;
}
