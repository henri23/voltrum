#pragma once

#include "core/asserts.hpp"
#include "defines.hpp"

#include "data_structures/auto_array.hpp"
#include "renderer/renderer_types.hpp"

#include <vulkan/vulkan.h>

#define VK_CHECK(expr) RUNTIME_ASSERT(expr == VK_SUCCESS);

// Forward declarations and enums
enum class Renderpass_Type {
    MAIN, // Main 3D scene rendering with depth buffer
    UI    // UI overlay rendering, color only
};

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

    f32 x, y, w, h;
    f32 r, g, b, a;

    f32 depth;
    u32 stencil;

    Renderpass_Type
        type; // Store renderpass type for attachment count optimization
    Renderpass_State state;
};

struct Vulkan_Framebuffer {
    VkFramebuffer handle;
    u32 attachment_count;
    VkImageView* attachments;
    Vulkan_Renderpass* renderpass;
};

// Off-screen rendering target (groups related resources like Vulkan_Swapchain)
// Uses same number of framebuffers as swapchain and synchronizes with
// image_index
struct Vulkan_Offscreen_Target {
    u32 width;
    u32 height;
    VkFormat color_format;
    VkFormat depth_format;

    // Dynamic count matching swapchain image_count
    u32 framebuffer_count;

    // Rendering attachments (one per swapchain image)
    Vulkan_Image* color_attachments;
    Vulkan_Image* depth_attachments;
    Vulkan_Framebuffer* framebuffers;

    // Texture sampling resources (one per swapchain image)
    VkSampler* samplers;
    VkDescriptorSet* descriptor_sets;
};

struct Vulkan_Swapchain {
    VkSwapchainKHR handle;
    u32 max_in_flight_frames;

    u32 image_count;
    VkImage* images;    // array of VkImages. Automatically created and cleaned
    VkImageView* views; // array of Views, struct that lets us access the images

    Auto_Array<Vulkan_Framebuffer> framebuffers;

    VkSurfaceFormatKHR image_format;
    VkExtent2D extent;

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

struct Vulkan_Fence {
    VkFence handle;
    b8 is_signaled;
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

constexpr const u32 OBJECT_SHADER_STAGE_COUNT = 2;

constexpr const u32 VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT = 2;

// NOTE: Max number of object. Will likelly change later
constexpr const u32 VULKAN_OBJECT_MAX_OBJECT_COUNT = 1024;

// TODO: I am assuming there will be for sure 3 swapchain images available
struct Vulkan_Descriptor_State {
    // One per image
    u32 generations[3];
};

struct Vulkan_Object_Shader_Object_State {
    // One descriptor set per vulkan image per object
    VkDescriptorSet descriptor_sets[3];

    Vulkan_Descriptor_State
        descriptor_states[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
};

struct Vulkan_Material_Shader {
    // The shader stage count is for vertex and fragment shaders
    Vulkan_Shader_Stage stages[OBJECT_SHADER_STAGE_COUNT];

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
    // TODO: Make dynamic
    Vulkan_Object_Shader_Object_State
        object_states[VULKAN_OBJECT_MAX_OBJECT_COUNT];

    Global_Uniform_Object global_ubo;
};

struct Vulkan_Context {
    f32 frame_delta_time;

    VkInstance instance;
    VkSurfaceKHR surface;
    VkAllocationCallbacks* allocator;
    VkPhysicalDevice
        physical_device; // Implicitly destroyed destroying VkInstance

    u32 framebuffer_width;
    u32 framebuffer_height;

    // Keep two replicas for last time and current. If these two are out of
    // sync, a resize event has ocurred
    u64 framebuffer_size_generation;
    u64 framebuffer_size_last_generation;

#ifdef DEBUG_BUILD
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

    u32 image_count;
    u32 image_index;
    u64 current_frame;

    b8 recreating_swapchain;

    Vulkan_Material_Shader material_shader;

    Vulkan_Swapchain swapchain;
    Vulkan_Device device;

    Vulkan_Renderpass main_renderpass;
    Vulkan_Renderpass ui_renderpass;

    Vulkan_Buffer object_vertex_buffer;
    Vulkan_Buffer object_index_buffer;

    // Main off-screen rendering target
    Vulkan_Offscreen_Target main_target;

    // Command buffers for rendering ui components
    Auto_Array<Vulkan_Command_Buffer> ui_graphics_command_buffers;

    // Main renderer command buffers for off-screen rendering
    Auto_Array<Vulkan_Command_Buffer> main_command_buffers;

    Auto_Array<VkSemaphore> image_available_semaphores;
    Auto_Array<VkSemaphore> render_finished_semaphores;

    u32 in_flight_fence_count;

    Auto_Array<Vulkan_Fence> in_flight_fences;
    // Keep information about the fences of the images currently in flight. The
    // fences are not owned by this array
    Auto_Array<Vulkan_Fence*> images_in_flight;

    // ImGui integration components
    VkDescriptorPool ui_descriptor_pool;
    VkDescriptorSetLayout ui_descriptor_set_layout;
    VkSampler ui_linear_sampler;

    u64 geometry_vertex_offset;
    u64 geometry_index_offset;

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
