#pragma once

#include "core/asserts.hpp"
#include "defines.hpp"

#include "data_structures/auto_array.hpp"
#include "renderer/renderer_types.hpp"
#include <vulkan/vulkan.h>

#define VK_CHECK(expr) RUNTIME_ASSERT(expr == VK_SUCCESS);

struct Vulkan_Buffer {
    u64 total_size;
    VkBuffer handle;
    VkBufferUsageFlags usage;
    b8 is_locked;
    VkDeviceMemory memory;
    s32 memory_index;
    u32 memory_property_flags;
};

struct Vulkan_Swapchain_Support_Info {
    VkSurfaceCapabilitiesKHR capabilities;
    u32 formats_count;
    VkSurfaceFormatKHR* formats;
    u32 present_modes_count;
    VkPresentModeKHR* present_modes;
};

struct Vulkan_Device {
    VkPhysicalDevice physical_device; // Handle ptr to physical device
    VkDevice logical_device;          // Handle to be destroyed

    // Family queue indices
    u32 graphics_queue_index;
    u32 transfer_queue_index;
    u32 compute_queue_index;
    u32 present_queue_index;

    b8 supports_device_local_host_visible;

    // Physical device informations
    VkPhysicalDeviceProperties physical_device_properties;
    VkPhysicalDeviceFeatures physical_device_features;
    VkPhysicalDeviceMemoryProperties physical_device_memory;

    Vulkan_Swapchain_Support_Info swapchain_info;

    VkFormat depth_format;

    // Queue handles
    VkQueue presentation_queue;
    VkQueue graphics_queue;
    VkQueue transfer_queue;

    VkCommandPool graphics_command_pool;
};

struct Vulkan_Image {
    VkImage handle;
    VkImageView view;
    VkDeviceMemory memory; // Handle to the memory allocated by the image
    u32 width;
    u32 height;
};

// Finite state machine of the renderpass
enum class Renderpass_State {
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDIN_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
};

struct Vulkan_Renderpass {
    VkRenderPass handle;

    vec4 render_area;
    vec4 clear_color;

    f32 depth;
    u32 stencil;

    u8 clear_flags;
    b8 has_prev;
    b8 has_next;

    Renderpass_State state;
};

// The swapchain will be owned by the ui library so no need to have a depth
// z-buffer
struct Vulkan_Swapchain {
    VkSwapchainKHR handle;
    u32 max_in_flight_frames;

    u32 image_count;
    VkImage* images;    // array of VkImages. Automatically created and cleaned
    VkImageView* views; // array of Views, struct that lets us access the images

    VkFramebuffer framebuffers[3];

    u32 framebuffer_width;
    u32 framebuffer_height;

    u64 framebuffer_size_generation;
    u64 framebuffer_size_last_generation;

    VkSurfaceFormatKHR image_format;
    VkExtent2D extent;
};

struct Vulkan_Viewport {
    VkImage* images;    // array of VkImages. Automatically created and cleaned
    VkImageView* views; // array of Views, struct that lets us access the images

    VkFramebuffer framebuffers[3];

    VkSurfaceFormatKHR image_format;
    VkExtent2D extent;

    u32 framebuffer_width;
    u32 framebuffer_height;

    u64 framebuffer_size_generation;
    u64 framebuffer_size_last_generation;

    Vulkan_Image depth_attachment;
};

enum class Command_Buffer_State {
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
};

struct Vulkan_Command_Buffer {
    VkCommandBuffer handle;

    Command_Buffer_State state;
};

struct Vulkan_Shader_Stage {
    VkShaderModuleCreateInfo create_info;
    VkShaderModule handle;
    VkPipelineShaderStageCreateInfo shader_stage_create_info;
};

struct Vulkan_Pipeline {
    VkPipeline handle;
    VkPipelineLayout pipeline_layout;
};

constexpr const u32 VULKAN_MATERIAL_SHADER_STAGE_COUNT = 2;
constexpr const u32 VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT = 2;
constexpr const u32 VULKAN_MATERIAL_SHADER_SAMPLER_COUNT = 1;

// NOTE: Max number of material instances
constexpr const u32 VULKAN_MAX_MATERIAL_COUNT = 1024;

// NOTE: Max number of simultaneously uploaded geometries
constexpr const u32 VULKAN_MAX_GEOMETRY_COUNT = 4096;

struct Vulkan_Geometry_Data {
    Geometry_ID id;
    u32 generation;
    u32 vertex_count;
    u32 vertex_size;
    u32 vertex_buffer_offset;
    u32 index_count;
    u32 index_size;
    u32 index_buffer_offset;
};

// TODO: I am assuming there will be for sure 3 swapchain images available
struct Vulkan_Descriptor_State {
    // One per image
    u32 generations[3];
    u32 ids[3];
};

struct Vulkan_Material_Shader_Object_State {
    // One descriptor set per vulkan image per object
    VkDescriptorSet descriptor_sets[3];

    Vulkan_Descriptor_State
        descriptor_states[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
};

struct Vulkan_Material_Shader_Global_Ubo {
    mat4 projection; // 64 bytes
    mat4 view;       // 64 bytes
    mat4 padding_0;  // 64 bytes
    mat4 padding_1;  // 64 bytes
};

// Local_Uniform_Object gets uploaded one per object per frame
struct Vulkan_Material_Shader_Instance_Ubo {
    vec4 diffuse_color;
    vec4 padding_0;
    vec4 padding_1;
    vec4 padding_2;
};

struct Vulkan_Material_Shader {
    // The shader stage count is for vertex and fragment shaders
    Vulkan_Shader_Stage stages[VULKAN_MATERIAL_SHADER_STAGE_COUNT];

    Vulkan_Pipeline pipeline;

    VkDescriptorPool global_descriptor_pool;
    VkDescriptorSetLayout global_descriptor_set_layout;
    // One descriptor set per vulkan_image
    // Dynamic arrays because I need 1 for each swapchain image
    VkDescriptorSet global_descriptor_sets[3];
    Vulkan_Buffer global_uniform_buffer;

    VkDescriptorPool object_descriptor_pool;
    VkDescriptorSetLayout object_descriptor_set_layout;

    // Allocate one large buffer to handle all objects
    Vulkan_Buffer object_uniform_buffer;

    // TODO: Manage a free list here
    u32 object_uniform_buffer_index;

    Texture_Type sampler_uses[VULKAN_MATERIAL_SHADER_SAMPLER_COUNT];

    // TODO: Make dynamic
    Vulkan_Material_Shader_Object_State
        object_states[VULKAN_MAX_MATERIAL_COUNT];

    Vulkan_Material_Shader_Global_Ubo global_ubo;
};

struct Vulkan_Context {
    f32 frame_delta_time;

    VkInstance instance;
    VkSurfaceKHR surface;
    VkAllocationCallbacks* allocator;
    VkPhysicalDevice
        physical_device; // Implicitly destroyed destroying VkInstance

#ifdef DEBUG_BUILD
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

    u32 image_count;
    u32 image_index;
    u64 current_frame;

    b8 recreating_swapchain;

    Vulkan_Material_Shader material_shader;

    Vulkan_Device device;

    // Swapchain is owned by the main renderpass which is owned by ImGui
    Vulkan_Swapchain swapchain;
    Vulkan_Viewport viewport;

    Vulkan_Renderpass viewport_renderpass;
    Vulkan_Renderpass ui_renderpass;

    Vulkan_Buffer object_vertex_buffer;
    Vulkan_Buffer object_index_buffer;

    // Command buffers for rendering ui components
    Auto_Array<Vulkan_Command_Buffer> command_buffers;

    Auto_Array<VkSemaphore> image_available_semaphores;
    Auto_Array<VkSemaphore> render_finished_semaphores;

    u32 in_flight_fence_count;
    VkFence in_flight_fences[2];

    // Keep information about the fences of the images currently in flight. The
    // fences are not owned by this array
    VkFence* images_in_flight[3];

    u64 geometry_vertex_offset;
    u64 geometry_index_offset;

    // TODO: Make dynamic
    Vulkan_Geometry_Data registered_geometries[VULKAN_MAX_GEOMETRY_COUNT];

    s32 (*find_memory_index)(u32 type_filter, u32 property_flags);
};

struct Vulkan_Texture_Data {
    Vulkan_Image image;
    VkSampler sampler;
};

struct Vulkan_Physical_Device_Requirements {
    b8 graphics;
    b8 present;
    b8 compute;
    b8 transfer;
    b8 discrete_gpu;
    b8 sampler_anisotropy;
    Auto_Array<const char*>* device_extension_names;
};

#define VK_DEVICE_LEVEL_FUNCTION(device, name)                                 \
    PFN_##name name = (PFN_##name)vkGetDeviceProcAddr(device, #name);          \
    RUNTIME_ASSERT_MSG(name, "Could not load device-level Vulkan function");

#define VK_INSTANCE_LEVEL_FUNCTION(instance, name)                             \
    PFN_##name name = (PFN_##name)vkGetInstanceProcAddr(instance, #name);      \
    RUNTIME_ASSERT_MSG(name, "Could not load instance-level Vulkan function");
