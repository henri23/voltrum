#include "vulkan_imgui_shader_pipeline.hpp"

#include "defines.hpp"

#include "renderer/vulkan/vulkan_buffer.hpp"
#include "renderer/vulkan/vulkan_pipeline.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

#include "core/logger.hpp"

#include "math/math_types.hpp"
#include "systems/texture_system.hpp"
#include <vulkan/vulkan_core.h>

// Include ImGui Vulkan backend to access shader modules
#include <imgui_impl_vulkan.h>

b8 vulkan_imgui_shader_pipeline_create(Vulkan_Context* context,
    Vulkan_ImGui_Shader_Pipeline* out_shader) {

    // Keep ImGui texture descriptor and sampler setup, but skip custom pipeline
    // creation so ImGui manages its own pipeline.
    // Global descriptors
    VkDescriptorSetLayoutBinding binding;
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.pImmutableSamplers = 0;
    // For ImGui all the content will be treated as textures and these textures
    // will be sampled textures with an associated sampler and this can be
    // accessed only by the fragment shader
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    VkDescriptorSetLayoutCreateInfo layout_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = 1;
    layout_info.pBindings = &binding;

    VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical_device,
        &layout_info,
        context->allocator,
        &out_shader->texture_descriptor_set_layout));

    VkDescriptorPoolSize pool_size;
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = VULKAN_IMGUI_SHADER_MAX_TEXTURE_COUNT;

    VkDescriptorPoolCreateInfo pool_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = VULKAN_IMGUI_SHADER_MAX_TEXTURE_COUNT;

    VK_CHECK(vkCreateDescriptorPool(context->device.logical_device,
        &pool_info,
        context->allocator,
        &out_shader->texture_descriptor_pool));

    constexpr u32 vertex_stride = 20; // sizeof(ImDrawVert)

    // Single descriptor set layout
    constexpr u32 descriptor_set_layout_count = 1;
    VkDescriptorSetLayout layouts[descriptor_set_layout_count] = {
        out_shader->texture_descriptor_set_layout};

    // Pipeline creation disabled while ImGui manages its own pipeline via
    // ImGui_ImplVulkan_RenderDrawData. Leave handles null to avoid custom
    // binds.
    out_shader->pipeline.handle = VK_NULL_HANDLE;
    out_shader->pipeline.pipeline_layout = VK_NULL_HANDLE;

    // Create linear sampler for ImGui textures
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.minLod = -1000;
    sampler_info.maxLod = 1000;
    sampler_info.maxAnisotropy = 1.0f;

    VK_CHECK(vkCreateSampler(context->device.logical_device,
        &sampler_info,
        context->allocator,
        &out_shader->texture_linear_sampler));

    return true;
}

void vulkan_imgui_shader_pipeline_destroy(Vulkan_Context* context,
    Vulkan_ImGui_Shader_Pipeline* shader) {

    VkDevice logical_device = context->device.logical_device;

    // Destroy descriptor resources
    vkDestroyDescriptorPool(logical_device,
        shader->texture_descriptor_pool,
        context->allocator);

    vkDestroyDescriptorSetLayout(logical_device,
        shader->texture_descriptor_set_layout,
        context->allocator);

    // Destroy sampler
    vkDestroySampler(logical_device,
        shader->texture_linear_sampler,
        context->allocator);

    // Destroy pipeline
    vulkan_graphics_pipeline_destroy(context, &shader->pipeline);

    // Note: No shader modules to destroy - ImGui manages its own
}

void vulkan_imgui_shader_pipeline_use(Vulkan_Context* context,
    Vulkan_ImGui_Shader_Pipeline* shader) {

    (void)context;
    (void)shader;
    // No-op: ImGui backend binds its own pipeline.
}
