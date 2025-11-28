#include "vulkan_shader_utils.hpp"

#include "core/logger.hpp"
#include "memory/memory.hpp"

#include "systems/resource_system.hpp"
#include "utils/string.hpp"

b8 create_shader_module(Vulkan_Context* context,
    const char* name,
    const char* type_str,
    VkShaderStageFlagBits shader_stage_flag,
    u32 stage_index,
    Vulkan_Shader_Stage* shader_stages) {

    char file_name[512];
    string_format(file_name, "shaders/%s.%s.spv", name, type_str);

    Resource binary_resource;
    if (!resource_system_load(file_name,
            Resource_Type::BINARY,
            &binary_resource)) {
        CORE_ERROR("Unable to read shader module: %s", file_name);
        return false;
    }

    memory_zero(&shader_stages[stage_index].create_info,
        sizeof(VkShaderModuleCreateInfo));

    shader_stages[stage_index].create_info.sType =
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_stages[stage_index].create_info.codeSize = binary_resource.data_size;
    shader_stages[stage_index].create_info.pCode =
        static_cast<u32*>(binary_resource.data);

    VK_CHECK(vkCreateShaderModule(context->device.logical_device,
        &shader_stages[stage_index].create_info,
        context->allocator,
        &shader_stages[stage_index].handle));

    CORE_DEBUG("Shader module created for %s - size: %zu bytes",
        file_name,
        binary_resource.data_size);

    resource_system_unload(&binary_resource);

    memory_zero(&shader_stages[stage_index].shader_stage_create_info,
        sizeof(VkPipelineShaderStageCreateInfo));

    shader_stages[stage_index].shader_stage_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[stage_index].shader_stage_create_info.stage =
        shader_stage_flag;
    shader_stages[stage_index].shader_stage_create_info.module =
        shader_stages[stage_index].handle;
    shader_stages[stage_index].shader_stage_create_info.pName = "main";

    return true;
}
