#include "binary_loader.hpp"
#include "core/asserts.hpp"
#include "core/logger.hpp"

// Include embedded font data
#include "fonts/jetbrains_bold.embed"
#include "fonts/jetbrains_italic.embed"
#include "fonts/jetbrains_regular.embed"

#include "fonts/roboto_bold.embed"
#include "fonts/roboto_italic.embed"
#include "fonts/roboto_regular.embed"

#include <cstring>

// Embedded asset structure
struct Embedded_Binary_Asset {
    const char* name;
    const u8* data;
    u64 size;
};

// Binary assets lookup table
internal_variable const Embedded_Binary_Asset binary_assets[] = {
    {"roboto_regular", roboto_regular, sizeof(roboto_regular)},
    {"roboto_bold", roboto_bold, sizeof(roboto_bold)},
    {"roboto_italic", roboto_italic, sizeof(roboto_italic)},

    {"jetbrains_regular", jetbrains_regular, sizeof(jetbrains_regular)},
    {"jetbrains_bold", jetbrains_bold, sizeof(jetbrains_bold)},
    {"jetbrains_italic", jetbrains_italic, sizeof(jetbrains_italic)},
};

constexpr u32 binary_asset_count =
    sizeof(binary_assets) / sizeof(binary_assets[0]);

// Find binary asset by name
INTERNAL_FUNC const Embedded_Binary_Asset* find_binary_asset(const char* name) {
    for (u32 i = 0; i < binary_asset_count; ++i) {
        if (strcmp(binary_assets[i].name, name) == 0) {
            return &binary_assets[i];
        }
    }
    return nullptr;
}

const u8* binary_loader_get_data(const char* asset_name, u64* out_size) {
    RUNTIME_ASSERT_MSG(asset_name, "Asset name cannot be null");
    RUNTIME_ASSERT_MSG(out_size, "Output size pointer cannot be null");

    const Embedded_Binary_Asset* asset = find_binary_asset(asset_name);
    if (!asset) {
        CORE_ERROR("Binary asset '%s' not found", asset_name);
        *out_size = 0;
        return nullptr;
    }

    *out_size = asset->size;
    CORE_DEBUG("Retrieved binary data: %s (%llu bytes)",
        asset_name,
        asset->size);
    return asset->data;
}
