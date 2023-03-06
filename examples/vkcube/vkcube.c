#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include <wsi/platform.h>
#include <wsi/window.h>
#include <wsi/vulkan.h>

#include <cglm/cglm.h>

#define array_size(a) (sizeof(a) / sizeof(a[0]))

#define NUM_FRAMES 3

struct vertex {
    vec3 pos;
    vec3 color;
    vec3 normal;
    vec2 uv;
};

struct mesh {
    struct vertex *vertices;
    uint32_t vertex_count;
    uint16_t *indices;
    uint32_t index_count;
};

struct demo {
    WsiPlatform           platform;
    WsiWindow             window;
    WsiExtent             window_extent;
    bool                  resized;
    bool                  closed;
    VkInstance            instance;
    VkPhysicalDevice      physical_device;
    uint32_t              graphics_family;
    uint32_t              present_family;
    VkDevice              device;
    VkQueue               graphics_queue;
    VkQueue               present_queue;
    VkSurfaceKHR          surface;
    VkSurfaceFormatKHR    surface_format;
    VkPresentModeKHR      present_mode;
    VkSwapchainKHR        swapchain;
    VkExtent2D            swapchain_extent;
    uint32_t              swapchain_image_count;
    VkImage               *swapchain_images;
    VkImageView           *swapchain_image_views;
    VkRenderPass          render_pass;
    VkFramebuffer         *frame_buffers;
    VkFence               fences[NUM_FRAMES];
    VkSemaphore           acquire_semaphores[NUM_FRAMES];
    VkSemaphore           render_semaphores[NUM_FRAMES];
    VkCommandPool         cmd_pool;
    VkCommandBuffer       cmd_buffers[NUM_FRAMES];
    uint32_t              image_index;
    uint32_t              frame_index;
    VkPipelineLayout      pipeline_layout;
    VkPipeline            pipeline;
    VkBuffer              vertex_buffer;
    VkDeviceMemory        vertex_memory;
    VkBuffer              index_buffer;
    VkDeviceMemory        index_memory;

    struct mesh mesh;
};

struct push_constants {
    mat4 mvp;
};

// region Extern

extern VkResult
demo_create_shader_module(VkDevice device, const char *path, VkShaderModule *module);

extern int64_t
demo_get_time_ns(void);

// endregion

// region Mesh

static void
mesh_generate(struct mesh *mesh)
{
    struct vertex vertices[] = {
        {{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
        {{ 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
        {{ 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
        {{ 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
        {{ 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
        {{ 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
        {{ 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
        {{ 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
    };

    uint16_t indices[] = {
        0, 2, 1,
        0, 3, 2,
        0, 1, 5,
        0, 5, 4,
        0, 4, 7,
        0, 7, 3,
        1, 2, 6,
        1, 6, 5,
        2, 3, 7,
        2, 7, 6,
        4, 5, 6,
        4, 6, 7,
    };

    mesh->vertex_count = array_size(vertices);
    mesh->vertices = malloc(sizeof(vertices));
    memcpy(mesh->vertices, vertices, sizeof(vertices));

    mesh->index_count = array_size(indices);
    mesh->indices = malloc(sizeof(indices));
    memcpy(mesh->indices, indices, sizeof(indices));
}

static void
mesh_destroy(struct mesh *mesh)
{
    free(mesh->vertices);
    mesh->vertices = NULL;
    mesh->vertex_count = 0;

    free(mesh->indices);
    mesh->indices = NULL;
    mesh->index_count = 0;
}

// endregion

// region Math

static void
perspective(float fovZ, float aspect, float yNear, float yFar, mat4 out)
{
    memset(out, 0, sizeof(mat4));

    out[0][0] = 1.0f / (tanf(fovZ / 2.0f) * aspect);
    out[2][1] = -1.0f / tanf(fovZ / 2.0f);
    out[1][2] = yFar / (yFar - yNear);
    out[1][3] = 1.0f;
    out[3][2] = (yNear * yFar) / (yNear - yFar);
}

void
lookat(vec3 eye, vec3 at, vec3 up, mat4 out)
{
    vec3 f;
    glm_vec3_sub(at, eye, f);
    glm_vec3_normalize(f);

    vec3 r;
    glm_vec3_cross(f, up, r);
    glm_vec3_normalize(r);

    vec3 u;
    glm_vec3_cross(r, f, u);

    out[0][0] = r[0];
    out[0][1] = f[0];
    out[0][2] = u[0];
    out[0][3] = 0.0f;

    out[1][0] = r[1];
    out[1][1] = f[1];
    out[1][2] = u[1];
    out[1][3] = 0.0f;

    out[2][0] = r[2];
    out[2][1] = f[2];
    out[2][2] = u[2];
    out[2][3] = 0.0f;

    out[3][0] = -glm_vec3_dot(r, eye);
    out[3][1] = -glm_vec3_dot(f, eye);
    out[3][2] = -glm_vec3_dot(u, eye);
    out[3][3] = 1.0f;
}

// endregion


static void
demo_resize(struct demo *demo);

static void
demo_begin_frame(struct demo *demo)
{
    uint32_t frame_index = demo->frame_index;

    VkResult res = vkWaitForFences(
        demo->device,
        1,
        &demo->fences[frame_index],
        VK_TRUE,
        UINT64_MAX);
    assert(res == VK_SUCCESS);

    res = vkResetFences(demo->device, 1, &demo->fences[frame_index]);
    assert(res == VK_SUCCESS);

    res = vkResetCommandBuffer(demo->cmd_buffers[frame_index], 0);
    assert(res == VK_SUCCESS);

    res = vkAcquireNextImageKHR(
        demo->device,
        demo->swapchain,
        UINT64_MAX,
        demo->acquire_semaphores[frame_index],
        VK_NULL_HANDLE,
        &demo->image_index);

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        demo_resize(demo);
    }
}

static void
demo_record_frame(struct demo *demo, float time)
{
    VkCommandBuffer cmd_buf = demo->cmd_buffers[demo->frame_index];

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };

    VkResult res = vkBeginCommandBuffer(cmd_buf, &begin_info);
    assert(res == VK_SUCCESS);

    VkClearValue clear_values[1];
    clear_values[0].color.float32[0] = 0.0f;
    clear_values[0].color.float32[1] = 0.0f;
    clear_values[0].color.float32[2] = 0.0f;
    clear_values[0].color.float32[3] = 0.8f;

    VkRect2D render_area = {
        .offset = { 0, 0 },
        .extent = demo->swapchain_extent,
    };

    VkRenderPassBeginInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = demo->render_pass,
        .framebuffer = demo->frame_buffers[demo->image_index],
        .renderArea = render_area,
        .clearValueCount = array_size(clear_values),
        .pClearValues = clear_values,
    };

    vkCmdBeginRenderPass(cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline);

    float wf = (float)demo->swapchain_extent.width;
    float hf = (float)demo->swapchain_extent.height;

    VkViewport viewport = { 0.0f, 0.0f, wf, hf, 0.0f, 1.0f };
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor = { { 0, 0 }, demo->swapchain_extent };
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    mat4 model = GLM_MAT4_IDENTITY_INIT;
    glm_rotate_x(model, time, model);
    glm_rotate_z(model, time, model);
    glm_translate(model, (vec3){ -0.5f, -0.5f, -0.5f });

    vec3 eye = { 0.0f, -3.0f, 0.0f };
    vec3 center = { 0.0f, 0.0f, 0.0f };
    vec3 up = { 0.0f, 0.0f, 1.0f };

    mat4 view;
    lookat(eye, center, up, view);

    float aspect = wf / hf;
    mat4 proj;
    perspective(glm_rad(45.0f), aspect, 0.1f, 10.0f, proj);

    struct push_constants push = {0};
    glm_mat4_mulN((mat4 *[]){&proj, &view, &model}, 3, push.mvp);

    vkCmdPushConstants(cmd_buf, demo->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
    vkCmdBindIndexBuffer(cmd_buf, demo->index_buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindVertexBuffers(cmd_buf, 0, 1, &demo->vertex_buffer, (VkDeviceSize[]){0 });
    vkCmdDrawIndexed(cmd_buf, demo->mesh.index_count, 1, 0, 0, 0);
    vkCmdEndRenderPass(cmd_buf);

    res = vkEndCommandBuffer(cmd_buf);
    assert(res == VK_SUCCESS);
}

static void
demo_end_frame(struct demo *demo)
{
    uint32_t frame_index = demo->frame_index;

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &demo->acquire_semaphores[frame_index],
        .pWaitDstStageMask = (VkPipelineStageFlags []) {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        },
        .commandBufferCount = 1,
        .pCommandBuffers = &demo->cmd_buffers[frame_index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &demo->render_semaphores[frame_index],
    };

    VkResult res = vkQueueSubmit(demo->graphics_queue, 1, &submit_info, demo->fences[frame_index]);
    assert(res == VK_SUCCESS);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &demo->render_semaphores[frame_index],
        .swapchainCount = 1,
        .pSwapchains = &demo->swapchain,
        .pImageIndices = &demo->image_index,
        .pResults = NULL,
    };

    res = vkQueuePresentKHR(demo->present_queue, &present_info);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        demo_resize(demo);
    }

    demo->frame_index = (demo->frame_index + 1) % NUM_FRAMES;
}


static uint32_t
demo_find_memory_type(VkPhysicalDevice dev, uint32_t filter, VkMemoryPropertyFlags flags)
{
    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(dev, &props);

    for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
        if ((filter & (1 << i)) && (props.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    return UINT32_MAX;
}

static void
demo_create_index_buffer(struct demo *demo)
{
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = demo->mesh.index_count * sizeof(uint16_t),
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
    };

    VkBuffer buffer;
    VkResult res = vkCreateBuffer(demo->device, &buffer_info, NULL, &buffer);
    assert(res == VK_SUCCESS);

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(demo->device, buffer, &mem_reqs);

    uint32_t index = demo_find_memory_type(
        demo->physical_device,
        mem_reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    assert(index != UINT32_MAX);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = index,
    };

    VkDeviceMemory memory;
    res = vkAllocateMemory(demo->device, &alloc_info, NULL, &memory);
    assert(res == VK_SUCCESS);

    res = vkBindBufferMemory(demo->device, buffer, memory, 0);
    assert(res == VK_SUCCESS);

    void *data;
    res = vkMapMemory(demo->device, memory, 0, mem_reqs.size, 0, &data);
    assert(res == VK_SUCCESS);
    memcpy(data, demo->mesh.indices, demo->mesh.index_count * sizeof(uint16_t));
    vkUnmapMemory(demo->device, memory);

    demo->index_buffer = buffer;
    demo->index_memory = memory;
}

static void
demo_destroy_index_buffer(struct demo *demo)
{
    vkDestroyBuffer(demo->device, demo->index_buffer, NULL);
    demo->index_buffer = VK_NULL_HANDLE;

    vkFreeMemory(demo->device, demo->index_memory, NULL);
    demo->index_memory = VK_NULL_HANDLE;
}

static void
demo_create_vertex_buffer(struct demo *demo)
{
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = demo->mesh.vertex_count * sizeof(struct vertex),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
    };

    VkBuffer buffer;
    VkResult res = vkCreateBuffer(demo->device, &info, NULL, &buffer);
    assert(res == VK_SUCCESS);

    VkMemoryRequirements req = {0};
    vkGetBufferMemoryRequirements(demo->device, buffer, &req);

    uint32_t index = demo_find_memory_type(
        demo->physical_device,
        req.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    assert(index != UINT32_MAX);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = req.size,
        .memoryTypeIndex = index,
    };

    VkDeviceMemory memory;
    res = vkAllocateMemory(demo->device, &alloc_info, NULL, &memory);
    assert(res == VK_SUCCESS);

    res = vkBindBufferMemory(demo->device, buffer, memory, 0);
    assert(res == VK_SUCCESS);

    void *data;
    res = vkMapMemory(demo->device, memory, 0, req.size, 0, &data);
    assert(res == VK_SUCCESS);
    memcpy(data, demo->mesh.vertices, demo->mesh.vertex_count * sizeof(struct vertex));
    vkUnmapMemory(demo->device, memory);

    demo->vertex_buffer = buffer;
    demo->vertex_memory = memory;
}

static void
demo_destroy_vertex_buffer(struct demo *demo)
{
    vkDestroyBuffer(demo->device, demo->vertex_buffer, NULL);
    demo->vertex_buffer = VK_NULL_HANDLE;

    vkFreeMemory(demo->device, demo->vertex_memory, NULL);
    demo->vertex_memory = VK_NULL_HANDLE;
}

static void
demo_create_pipeline(struct demo *demo)
{
    // region Shader Modules

    VkPipelineShaderStageCreateInfo shader_stages[2] = {0};

    VkShaderModule vert_module;
    VkResult res = demo_create_shader_module(demo->device, "vkcube.vert.spv", &vert_module);
    assert(res == VK_SUCCESS);

    shader_stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vert_module;
    shader_stages[0].pName  = "main";

    VkShaderModule frag_module;
    res = demo_create_shader_module(demo->device, "vkcube.frag.spv", &frag_module);
    assert(res == VK_SUCCESS);

    shader_stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = frag_module;
    shader_stages[1].pName  = "main";

    // endregion

    // region Vertex Input

    VkVertexInputBindingDescription binding_desc[] = {
        { 0, sizeof(struct vertex), VK_VERTEX_INPUT_RATE_VERTEX },
    };

    VkVertexInputAttributeDescription attribute_desc[] = {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(struct vertex, pos)},
        { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(struct vertex, color)},
        { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(struct vertex, normal)},
        { 3, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(struct vertex, uv)},
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .vertexBindingDescriptionCount = array_size(binding_desc),
        .pVertexBindingDescriptions = binding_desc,
        .vertexAttributeDescriptionCount = array_size(attribute_desc),
        .pVertexAttributeDescriptions = attribute_desc,
    };

    // endregion

    // region Assembly

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // endregion

    // region Tesselation

    VkPipelineTessellationStateCreateInfo tessellation_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .patchControlPoints = 0,
    };

    // endregion

    // region Viewport State

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pViewports = NULL,
        .viewportCount = 1,
        .pScissors = NULL,
        .scissorCount = 1,
    };

    // endregion

    // region Rasterization

    VkPipelineRasterizationStateCreateInfo rasterization_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
        .lineWidth = 1.0f,
    };

    // endregion

    // region Multisample

    VkPipelineMultisampleStateCreateInfo multisample_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    // endregion

    // region Depth Stencil

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    // endregion

    // region Color Blend

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                        | VK_COLOR_COMPONENT_G_BIT
                        | VK_COLOR_COMPONENT_B_BIT
                        | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state,
        .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
    };

    // endregion

    // region Dynamic State

    VkDynamicState states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .dynamicStateCount = array_size(states),
        .pDynamicStates = states,
    };

    // endregion

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stageCount = array_size(shader_stages),
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState = &tessellation_state,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState = &multisample_state,
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &dynamic_state,
        .layout = demo->pipeline_layout,
        .renderPass = demo->render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    };

    VkPipeline pipeline;
    res = vkCreateGraphicsPipelines(
        demo->device,
        VK_NULL_HANDLE,
        1,
        &pipeline_create_info,
        NULL,
        &pipeline);
    assert(res == VK_SUCCESS);

    vkDestroyShaderModule(demo->device, vert_module, NULL);
    vkDestroyShaderModule(demo->device, frag_module, NULL);

    demo->pipeline = pipeline;
}

static void
demo_destroy_pipeline(struct demo *demo)
{
    vkDestroyPipeline(demo->device, demo->pipeline, NULL);
    demo->pipeline = VK_NULL_HANDLE;
}

static void
demo_create_pipeline_layout(struct demo *demo)
{
    VkPushConstantRange ranges[] = {
        { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(struct push_constants) },
    };

    VkPipelineLayoutCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = array_size(ranges),
        .pPushConstantRanges = ranges,
    };

    VkPipelineLayout layout;
    VkResult result = vkCreatePipelineLayout(demo->device, &create_info, NULL, &layout);
    assert(result == VK_SUCCESS);
    demo->pipeline_layout = layout;
}

static void
demo_destroy_pipeline_layout(struct demo *demo)
{
    vkDestroyPipelineLayout(demo->device, demo->pipeline_layout, NULL);
    demo->pipeline_layout = VK_NULL_HANDLE;
}

static void
demo_create_frame_buffers(struct demo *demo)
{
    demo->frame_buffers = calloc(demo->swapchain_image_count, sizeof(VkFramebuffer));
    assert(demo->frame_buffers);

    for (uint32_t i = 0; i < demo->swapchain_image_count; i++) {
        VkImageView attachments[] = {
            demo->swapchain_image_views[i],
        };

        VkFramebufferCreateInfo fb_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .renderPass = demo->render_pass,
            .attachmentCount = array_size(attachments),
            .pAttachments = attachments,
            .width = demo->swapchain_extent.width,
            .height = demo->swapchain_extent.height,
            .layers = 1,
        };

        VkResult res = vkCreateFramebuffer(demo->device, &fb_info, NULL, &demo->frame_buffers[i]);
        assert(res == VK_SUCCESS);
    }
}

static void
demo_destroy_frame_buffers(struct demo *demo)
{
    for (uint32_t i = 0; i < demo->swapchain_image_count; i++) {
        vkDestroyFramebuffer(demo->device, demo->frame_buffers[i], NULL);
    }
    free(demo->frame_buffers);
    demo->frame_buffers = NULL;
}

static void
demo_create_renderpass(struct demo *demo)
{
    VkAttachmentDescription attachments[1];
    attachments[0].flags = 0;
    attachments[0].format = demo->surface_format.format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_refs[1] = {
        { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
    };

    VkSubpassDescription subpasses[1];
    subpasses[0].flags = 0;
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].inputAttachmentCount = 0;
    subpasses[0].pInputAttachments = NULL;
    subpasses[0].colorAttachmentCount = array_size(color_refs),
    subpasses[0].pColorAttachments = color_refs;
    subpasses[0].pResolveAttachments = NULL;
    subpasses[0].pDepthStencilAttachment = NULL;
    subpasses[0].preserveAttachmentCount = 0;
    subpasses[0].pPreserveAttachments = NULL;

    VkRenderPassCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = array_size(attachments),
        .pAttachments = attachments,
        .subpassCount = array_size(subpasses),
        .pSubpasses = subpasses,
        .dependencyCount = 0,
        .pDependencies = NULL,
    };

    VkResult result = vkCreateRenderPass(demo->device, &create_info, NULL, &demo->render_pass);
    assert(result == VK_SUCCESS);
}

static void
demo_destroy_renderpass(struct demo *demo)
{
    vkDestroyRenderPass(demo->device, demo->render_pass, NULL);
    demo->render_pass = VK_NULL_HANDLE;
}

static void
demo_create_color_images(struct demo *demo)
{
    uint32_t image_count;
    VkResult res =  vkGetSwapchainImagesKHR(demo->device, demo->swapchain, &image_count, NULL);
    assert(res == VK_SUCCESS);

    VkImage *images = calloc(image_count, sizeof(VkImage));
    assert(images);

    res = vkGetSwapchainImagesKHR(demo->device, demo->swapchain, &image_count, images);
    assert(res == VK_SUCCESS);

    VkImageView *image_views = calloc(image_count, sizeof(VkImageView));
    assert(image_views);

    for (uint32_t i = 0; i < image_count; i++) {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = demo->surface_format.format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };

        VkResult result = vkCreateImageView(demo->device, &view_info, NULL, &image_views[i]);
        assert(result == VK_SUCCESS);
    }

    demo->swapchain_images = images;
    demo->swapchain_image_count = image_count;
    demo->swapchain_image_views = image_views;
}

static void
demo_destroy_color_images(struct demo *demo)
{
    for (uint32_t i = 0; i < demo->swapchain_image_count; i++) {
        vkDestroyImageView(demo->device, demo->swapchain_image_views[i], NULL);
    }

    free(demo->swapchain_image_views);
    demo->swapchain_image_views = NULL;

    free(demo->swapchain_images);
    demo->swapchain_images = NULL;
}

static void
demo_select_present_mode(struct demo *demo, bool unlocked, bool tearing)
{
    VkPresentModeKHR modes[8];
    uint32_t mode_count = array_size(modes);

    VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        demo->physical_device,
        demo->surface,
        &mode_count,
        modes);
    assert(result == VK_SUCCESS);

    VkPresentModeKHR priority[3];
    uint32_t priority_count = 0;

    if (unlocked) {
        if (tearing) {
            priority[priority_count++] = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
        priority[priority_count++] = VK_PRESENT_MODE_MAILBOX_KHR;
    }
    if (tearing) {
        priority[priority_count++] = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    }

    VkPresentModeKHR selected = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < priority_count; i++) {
        for (uint32_t j = 0; j < mode_count; j++) {
            if (modes[j] == priority[i]) {
                selected = modes[j];
                goto done;
            }
        }
    }

done:
    demo->present_mode = selected;
}

static void
demo_select_surface_format(struct demo *demo)
{
    VkSurfaceFormatKHR formats[32];
    uint32_t format_count = array_size(formats);

    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        demo->physical_device,
        demo->surface,
        &format_count,
        formats);
    assert(result == VK_SUCCESS);

    for (uint32_t i = 0; i < format_count; ++i) {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) {
            demo->surface_format = formats[i];
        }
    }

    demo->surface_format = formats[0];
}

static void
demo_create_swapchain(struct demo *demo, VkSwapchainKHR oldSwapchain)
{
    VkSurfaceCapabilitiesKHR surface_caps;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        demo->physical_device,
        demo->surface,
        &surface_caps);

    VkExtent2D extent = surface_caps.currentExtent;
    if (extent.width == UINT32_MAX || extent.height == UINT32_MAX) {
        extent.width = (uint32_t)demo->window_extent.width;
        extent.height = (uint32_t)demo->window_extent.height;
    }
    demo->swapchain_extent = extent;

    VkSwapchainCreateInfoKHR info = {0};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = demo->surface;

    if (surface_caps.minImageCount > NUM_FRAMES) {
        info.minImageCount = surface_caps.minImageCount;
    } else {
        info.minImageCount = NUM_FRAMES;
    }

    info.imageFormat = demo->surface_format.format;
    info.imageColorSpace = demo->surface_format.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (demo->graphics_family == demo->present_family) {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount = 0;
        info.pQueueFamilyIndices = NULL;
    } else {
        uint32_t indices[] = {
            demo->graphics_family,
            demo->present_family,
        };
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = array_size(indices);
        info.pQueueFamilyIndices = indices;
    }

    info.preTransform = surface_caps.currentTransform;

    if (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
        info.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    } else {
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }

    info.presentMode = demo->present_mode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = oldSwapchain;

    result = vkCreateSwapchainKHR(demo->device, &info, NULL, &demo->swapchain);
    assert(result == VK_SUCCESS);
}

static void
demo_destroy_swapchain(struct demo *demo, VkSwapchainKHR *pSwapchain)
{
    if (pSwapchain != NULL) {
        *pSwapchain = demo->swapchain;
    } else {
        vkDestroySwapchainKHR(demo->device, demo->swapchain, NULL);
    }
    demo->swapchain = VK_NULL_HANDLE;
}

static void
demo_create_command_buffers(struct demo *demo)
{
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = demo->graphics_family,
    };

    VkResult res = vkCreateCommandPool(demo->device, &pool_info, NULL, &demo->cmd_pool);
    assert(res == VK_SUCCESS);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = demo->cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = NUM_FRAMES,
    };

    vkAllocateCommandBuffers(demo->device, &alloc_info, demo->cmd_buffers);
    assert(res == VK_SUCCESS);
}

static void
demo_destroy_command_buffers(struct demo *demo)
{
    vkFreeCommandBuffers(demo->device, demo->cmd_pool, NUM_FRAMES, demo->cmd_buffers);
    for (uint32_t i = 0; i < NUM_FRAMES; ++i) {
        demo->cmd_buffers[i] = NULL;
    }

    vkDestroyCommandPool(demo->device, demo->cmd_pool, NULL);
    demo->cmd_pool = VK_NULL_HANDLE;
}

static void
demo_create_sync_objects(struct demo *demo)
{
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };

    for (int i = 0; i < NUM_FRAMES; ++i) {
        VkResult res = vkCreateFence(demo->device, &fence_info, NULL, &demo->fences[i]);
        assert(res == VK_SUCCESS);

        res = vkCreateSemaphore(demo->device, &semaphore_info, NULL, &demo->acquire_semaphores[i]);
        assert(res == VK_SUCCESS);

        res = vkCreateSemaphore(demo->device, &semaphore_info, NULL, &demo->render_semaphores[i]);
        assert(res == VK_SUCCESS);
    }
}

static void
demo_destroy_sync_objects(struct demo *demo)
{
    for (int i = 0; i < NUM_FRAMES; ++i) {
        vkDestroyFence(demo->device, demo->fences[i], NULL);
        demo->fences[i] = VK_NULL_HANDLE;

        vkDestroySemaphore(demo->device, demo->acquire_semaphores[i], NULL);
        demo->acquire_semaphores[i] = VK_NULL_HANDLE;

        vkDestroySemaphore(demo->device, demo->render_semaphores[i], NULL);
        demo->render_semaphores[i] = VK_NULL_HANDLE;
    }
}

static void
demo_create_device(struct demo *demo)
{
    const char *exts[16];
    uint32_t ext_count = array_size(exts);

    WsiResult wr = wsiEnumerateRequiredDeviceExtensions(demo->platform, &ext_count, exts);
    assert(wr == WSI_SUCCESS);

    uint32_t queue_count = 1;
    float queue_priorities[1] = { 1.0f };
    VkDeviceQueueCreateInfo queue_infos[2];

    queue_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_infos[0].pNext = NULL;
    queue_infos[0].flags = 0;
    queue_infos[0].queueFamilyIndex = demo->graphics_family;
    queue_infos[0].queueCount = 1;
    queue_infos[0].pQueuePriorities = queue_priorities;

    if (demo->graphics_family != demo->present_family) {
        queue_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_infos[1].pNext = NULL;
        queue_infos[1].flags = 0;
        queue_infos[1].queueFamilyIndex = demo->present_family;
        queue_infos[1].queueCount = 1;
        queue_infos[1].pQueuePriorities = queue_priorities;
        ++queue_count;
    }

    VkPhysicalDeviceFeatures features = {0};
    features.fillModeNonSolid = VK_TRUE;

    VkDeviceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = queue_count,
        .pQueueCreateInfos = queue_infos,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = ext_count,
        .ppEnabledExtensionNames = exts,
        .pEnabledFeatures = &features,
    };

    VkResult res = vkCreateDevice(demo->physical_device, &info, NULL, &demo->device);
    assert(res == VK_SUCCESS);

    vkGetDeviceQueue(demo->device, demo->graphics_family, 0, &demo->graphics_queue);
    vkGetDeviceQueue(demo->device, demo->present_family, 0, &demo->present_queue);
}

static void
demo_destroy_device(struct demo *demo)
{
    vkDestroyDevice(demo->device, NULL);

    demo->device = NULL;
    demo->graphics_queue = NULL;
    demo->present_queue = NULL;
}

static void
demo_select_physical_device(struct demo *demo)
{
    const uint32_t MAX_DEV_COUNT = 8;
    const uint32_t MAX_QUEUE_COUNT = 16;

    uint32_t dev_count = MAX_DEV_COUNT;
    VkPhysicalDevice devices[MAX_DEV_COUNT];
    VkResult res = vkEnumeratePhysicalDevices(demo->instance, &dev_count, devices);
    assert(res == VK_SUCCESS || res == VK_INCOMPLETE);

    for (uint32_t i = 0; i < dev_count; ++i) {
        VkPhysicalDevice device = devices[i];

        uint32_t queue_count = MAX_QUEUE_COUNT;
        VkQueueFamilyProperties queue_family_props[MAX_QUEUE_COUNT];
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, queue_family_props);

        uint32_t graphics_family = VK_QUEUE_FAMILY_IGNORED;
        uint32_t present_family = VK_QUEUE_FAMILY_IGNORED;

        for (uint32_t j = 0; j < queue_count; ++j) {
            if (queue_family_props[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_family = j;
                break;
            }
        }

        for (uint32_t j = 0; j < queue_count; ++j) {
            VkBool32 supports_present = wsiGetPhysicalDevicePresentationSupport(
                demo->platform,
                device,
                j);

            if (supports_present) {
                present_family = j;
                break;
            }
        }

        if (graphics_family == VK_QUEUE_FAMILY_IGNORED ||
            present_family == VK_QUEUE_FAMILY_IGNORED) {
            continue;
        }

        demo->graphics_family = graphics_family;
        demo->present_family = present_family;
        demo->physical_device = device;
        break;
    }

    assert(demo->physical_device != NULL);
}

static void
demo_close_window(void *pUserData, const WsiCloseWindowEvent *pEvent)
{
    struct demo *demo = pUserData;
    demo->closed = true;
}

static void
demo_configure_window(void *pUserData, const WsiConfigureWindowEvent *pEvent)
{
    struct demo *demo = pUserData;
    if (demo->window_extent.width != pEvent->extent.width ||
        demo->window_extent.height != pEvent->extent.height)
    {
        demo->window_extent = pEvent->extent;
        demo->resized = true;
    }
}

static void
demo_create_window(struct demo *demo)
{
    WsiWindowCreateInfo window_info = {
        .sType = WSI_STRUCTURE_TYPE_WINDOW_CREATE_INFO,
        .pNext = NULL,
        .extent.width = 600,
        .extent.height = 600,
        .pTitle = "VkCube",
        .pUserData = demo,
        .pfnCloseWindow = demo_close_window,
        .pfnConfigureWindow = demo_configure_window,
    };

    WsiResult res = wsiCreateWindow(demo->platform, &window_info, &demo->window);
    assert(res == WSI_SUCCESS);

    res = wsiCreateWindowSurface(demo->window, demo->instance, NULL, &demo->surface);
    assert(res == WSI_SUCCESS);
}

static void
demo_destroy_window(struct demo *demo)
{
    vkDestroySurfaceKHR(demo->instance, demo->surface, NULL);
    demo->surface = VK_NULL_HANDLE;

    wsiDestroyWindow(demo->window);
    demo->window = NULL;
}

static void
demo_create_instance(struct demo *demo)
{
    const char *inst_exts[16];
    uint32_t inst_ext_count = array_size(inst_exts);

    WsiResult res = wsiEnumerateRequiredInstanceExtensions(
        demo->platform,
        &inst_ext_count,
        inst_exts);
    assert(res == WSI_SUCCESS);

    const char *inst_layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };
    uint32_t inst_layer_count = array_size(inst_layers);

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .apiVersion = VK_API_VERSION_1_0,
        .pApplicationName = "VkCube",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "VkCube",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
    };

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = inst_layer_count,
        .ppEnabledLayerNames = inst_layers,
        .enabledExtensionCount = inst_ext_count,
        .ppEnabledExtensionNames = inst_exts,
    };

    VkResult result = vkCreateInstance(&instance_info, NULL, &demo->instance);
    assert(result == VK_SUCCESS);
}

static void
demo_destroy_instance(struct demo *demo)
{
    vkDestroyInstance(demo->instance, NULL);
    demo->instance = NULL;
}

static void
demo_create_platform(struct demo *demo)
{
    WsiPlatformCreateInfo info = {
        .sType = WSI_STRUCTURE_TYPE_PLATFORM_CREATE_INFO,
        .pNext = NULL,
    };

    WsiResult res = wsiCreatePlatform(&info, &demo->platform);
    assert(res == WSI_SUCCESS);
}

static void
demo_destroy_platform(struct demo *demo)
{
    wsiDestroyPlatform(demo->platform);
    demo->platform = NULL;
}

static void
demo_resize(struct demo *demo)
{
    VkSwapchainKHR oldSwapchain;

    VkResult res = vkDeviceWaitIdle(demo->device);
    assert(res == VK_SUCCESS);

    demo_destroy_frame_buffers(demo);
    demo_destroy_color_images(demo);
    demo_destroy_swapchain(demo, &oldSwapchain);
    demo_create_swapchain(demo, oldSwapchain);
    demo_create_color_images(demo);
    demo_create_frame_buffers(demo);

    vkDestroySwapchainKHR(demo->device, oldSwapchain, NULL);
}

int
main(int argc, char **argv)
{
    struct demo demo = {0};
    demo_create_platform(&demo);
    demo_create_instance(&demo);
    demo_create_window(&demo);
    demo_select_physical_device(&demo);
    demo_create_device(&demo);
    demo_create_sync_objects(&demo);
    demo_create_command_buffers(&demo);
    demo_select_surface_format(&demo);
    demo_select_present_mode(&demo, true, false);
    demo_create_swapchain(&demo, VK_NULL_HANDLE);
    demo_create_color_images(&demo);
    demo_create_renderpass(&demo);
    demo_create_frame_buffers(&demo);
    demo_create_pipeline_layout(&demo);
    demo_create_pipeline(&demo);

    mesh_generate(&demo.mesh);

    demo_create_vertex_buffer(&demo);
    demo_create_index_buffer(&demo);

    int64_t last_time = demo_get_time_ns();

    float timef = 0.0f;
    while (true) {
        if (demo.closed) {
            break;
        }

        if (demo.resized) {
            demo_resize(&demo);
            demo.resized = false;
        }

        demo_begin_frame(&demo);
        demo_record_frame(&demo, timef);
        demo_end_frame(&demo);

        int64_t now = demo_get_time_ns();

        int64_t dt = now - last_time;
        last_time = now;

        timef += (float)dt / 1000000000.0f;
        timef = fmodf(timef, 6.28318530718f);

        WsiResult res = wsiDispatchEvents(demo.platform, 0);
        if (res != WSI_SUCCESS) {
            break;
        }
    }

    vkDeviceWaitIdle(demo.device);

    demo_destroy_index_buffer(&demo);
    demo_destroy_vertex_buffer(&demo);

    mesh_destroy(&demo.mesh);

    demo_destroy_pipeline(&demo);
    demo_destroy_pipeline_layout(&demo);
    demo_destroy_frame_buffers(&demo);
    demo_destroy_renderpass(&demo);
    demo_destroy_color_images(&demo);
    demo_destroy_swapchain(&demo, NULL);
    demo_destroy_command_buffers(&demo);
    demo_destroy_sync_objects(&demo);
    demo_destroy_device(&demo);
    demo_destroy_window(&demo);
    demo_destroy_instance(&demo);
    demo_destroy_platform(&demo);

    return 0;
}
