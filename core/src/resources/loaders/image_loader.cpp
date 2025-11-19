#include "image_loader.hpp"
#include "core/asserts.hpp"
#include "core/logger.hpp"

// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Include embedded image data
#include "icons/voltrum_icon.embed"
#include "images/window_images.embed"
#include <cstring>

// Embedded asset structure
struct Embedded_Image_Asset {
    const char* name;
    const u8* data;
    u64 size;
};

// Image assets lookup table
internal_variable const Embedded_Image_Asset image_assets[] = {
    {"voltrum_icon", voltrum_icon, sizeof(voltrum_icon)},
    {"window_minimize", window_minimize_icon, sizeof(window_minimize_icon)},
    {"window_maximize", window_maximize_icon, sizeof(window_maximize_icon)},
    {"window_restore", window_restore_icon, sizeof(window_restore_icon)},
    {"window_close", window_close_icon, sizeof(window_close_icon)},
};

constexpr u32 image_asset_count =
    sizeof(image_assets) / sizeof(image_assets[0]);

// Find image asset by name
INTERNAL_FUNC const Embedded_Image_Asset* find_image_asset(const char* name) {
    for (u32 i = 0; i < image_asset_count; ++i) {
        if (strcmp(image_assets[i].name, name) == 0) {
            return &image_assets[i];
        }
    }
    return nullptr;
}

Image_Load_Result image_loader_load(const char* image_name) {
    Image_Load_Result result = {};

    RUNTIME_ASSERT_MSG(image_name, "Image name cannot be null");

    const Embedded_Image_Asset* asset = find_image_asset(image_name);
    if (!asset) {
        result.success = false;
        result.error_message = "Image asset not found";
        CORE_ERROR("Image asset '%s' not found", image_name);
        return result;
    }

    // Decode the image data using stb_image
    s32 width, height, channels;
    u8* pixel_data = stbi_load_from_memory(asset->data,
        (s32)asset->size,
        &width,
        &height,
        &channels,
        STBI_rgb_alpha);

    if (!pixel_data) {
        result.success = false;
        result.error_message = stbi_failure_reason();
        CORE_ERROR("Failed to decode image asset '%s': %s",
            image_name,
            stbi_failure_reason());
        return result;
    }

    // Calculate pixel data size
    u32 pixel_data_size = width * height * 4; // RGBA

    result.success = true;
    result.error_message = nullptr;
    result.width = (u32)width;
    result.height = (u32)height;
    result.channels = (u32)channels;
    result.pixel_data = pixel_data;
    result.pixel_data_size = pixel_data_size;

    CORE_DEBUG("Loaded image asset: %s (%dx%d, %d channels)",
        image_name,
        width,
        height,
        channels);
    return result;
}
