#include "ui_fonts.hpp"

#include "data_structures/auto_array.hpp"
#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "imgui.h"
#include "resources/loaders/binary_loader.hpp"
#include <cstdio>
#include <cstring>

// Font information structure (internal only)
struct UI_Font_Info {
    const char* name;
    const char* family;
    UI_Font_Weight weight;
    UI_Font_Style style;
    f32 size;
    const u8* data;
    u32 data_size;
    ImFont* imgui_font;
    b8 is_loaded;
    b8 is_default;

    // Equality operator for Auto_Array find functionality
    bool operator==(const UI_Font_Info& other) const {
        return name && other.name && strcmp(name, other.name) == 0;
    }
};

// Font registry (internal only)
struct UI_Font_Registry {
    Auto_Array<UI_Font_Info> fonts;
    ImFont* default_font;
    b8 is_initialized;
};

// Internal font system state
internal_var UI_Font_Registry font_registry = {};

// Default embedded font data (placeholder - in real implementation would be
// actual font data) For now, we'll use ImGui's default font
internal_var const u8 default_font_data[] = {0}; // Placeholder
internal_var const u32 default_font_data_size = 0;

// Font name storage pool (to avoid stack memory issues)
constexpr u32 MAX_FONT_NAME_COUNT = 128;
constexpr u32 FONT_NAME_LENGTH = 64;
internal_var char font_name_pool[MAX_FONT_NAME_COUNT][FONT_NAME_LENGTH] =
    {};
internal_var u32 font_name_pool_index = 0;

// Internal functions
INTERNAL_FUNC UI_Font_Info* find_font_by_name(const char* name);
INTERNAL_FUNC ImFontConfig create_font_config(const UI_Font_Info* font_info);

b8 ui_fonts_initialize() {
    if (font_registry.is_initialized) {
        CORE_WARN("Font system already initialized");
        return true;
    }

    // Initialize font registry (Auto_Array handles its own memory)
    font_registry.fonts.clear();
    font_registry.default_font = nullptr;
    font_registry.is_initialized = true;

    // Reset font name pool
    font_name_pool_index = 0;

    CORE_INFO("Font management system initialized");
    return true;
}

b8 ui_fonts_register_embedded(const char* name,
    const char* family,
    UI_Font_Weight weight,
    UI_Font_Style style,
    const u8* data,
    u32 data_size,
    f32 size) {
    RUNTIME_ASSERT_MSG(name, "Font name cannot be null");
    RUNTIME_ASSERT_MSG(family, "Font family cannot be null");
    // Note: data can be null for default font
    if (data != nullptr) {
        RUNTIME_ASSERT_MSG(data_size > 0,
            "Font data size must be positive when data is provided");
    }

    if (!font_registry.is_initialized) {
        CORE_ERROR("Font system not initialized");
        return false;
    }

    // Check if font already exists
    if (find_font_by_name(name)) {
        CORE_WARN("Font '%s' already registered", name);
        return false;
    }

    // Create and add font info
    UI_Font_Info font_info = {name,
        family,
        weight,
        style,
        size,
        data,
        data_size,
        nullptr,
        false,
        false};

    font_registry.fonts.push_back(font_info);

    return true;
}

b8 ui_fonts_register_system(const char* name,
    const char* family,
    UI_Font_Weight weight,
    UI_Font_Style style,
    const char* filepath,
    f32 size) {
    RUNTIME_ASSERT_MSG(name, "Font name cannot be null");
    RUNTIME_ASSERT_MSG(family, "Font family cannot be null");
    RUNTIME_ASSERT_MSG(filepath, "Font filepath cannot be null");

    if (!font_registry.is_initialized) {
        CORE_ERROR("Font system not initialized");
        return false;
    }

    // Check if font already exists
    if (find_font_by_name(name)) {
        CORE_WARN("Font '%s' already registered", name);
        return false;
    }

    // Create and add font info for system font
    UI_Font_Info font_info = {name,
        family,
        weight,
        style,
        size,
        (const u8*)filepath, // Store filepath for system fonts
        0,                   // 0 indicates system font
        nullptr,
        false,
        false};

    font_registry.fonts.push_back(font_info);

    return true;
}

b8 ui_fonts_load_all() {
    if (!font_registry.is_initialized) {
        CORE_ERROR("Font system not initialized");
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();

    // Load each registered font
    for (u32 i = 0; i < font_registry.fonts.size(); ++i) {
        UI_Font_Info* font_info = &font_registry.fonts[i];

        if (font_info->is_loaded) {
            continue; // Skip already loaded fonts
        }

        ImFontConfig config = create_font_config(font_info);

        if (font_info->data && font_info->data_size > 0) {
            // Load embedded font from memory
            font_info->imgui_font =
                io.Fonts->AddFontFromMemoryTTF((void*)font_info->data,
                    font_info->data_size,
                    font_info->size,
                    &config);
        } else if (font_info->data && font_info->data_size == 0) {
            // Load system font from file (filepath stored in data pointer)
            const char* filepath = (const char*)font_info->data;
            font_info->imgui_font = io.Fonts->AddFontFromFileTTF(filepath,
                font_info->size,
                &config);
        } else {
            // Use default font with custom size
            font_info->imgui_font = io.Fonts->AddFontDefault(&config);
        }

        if (font_info->imgui_font) {
            font_info->is_loaded = true;
            if (font_info->size > 0.0f) {
                CORE_INFO("Font '%s' loaded (%.1fpt)",
                    font_info->name,
                    font_info->size);
            } else {
                CORE_INFO("Font '%s' loaded", font_info->name);
            }
        } else {
            CORE_ERROR("Failed to load font: %s", font_info->name);
        }
    }

    // Build font atlas
    if (!io.Fonts->Build()) {
        CORE_ERROR("Failed to build font atlas");
        return false;
    }

    CORE_INFO("Successfully loaded %d fonts", font_registry.fonts.size());
    return true;
}

b8 ui_fonts_set_default(const char* name) {
    RUNTIME_ASSERT_MSG(name, "Font name cannot be null");

    UI_Font_Info* font_info = find_font_by_name(name);
    if (!font_info) {
        CORE_ERROR("Font '%s' not found", name);
        return false;
    }

    if (!font_info->is_loaded) {
        CORE_ERROR("Font '%s' not loaded", name);
        return false;
    }

    // Clear previous default
    for (u32 i = 0; i < font_registry.fonts.size(); ++i) {
        font_registry.fonts[i].is_default = false;
    }

    // Set new default
    font_info->is_default = true;
    font_registry.default_font = font_info->imgui_font;

    // Set ImGui default font
    ImGuiIO& io = ImGui::GetIO();
    io.FontDefault = font_registry.default_font;

    CORE_INFO("Default font set to '%s'", name);
    return true;
}

ImFont* ui_fonts_find_by_style(const char* family,
    UI_Font_Weight weight,
    UI_Font_Style style) {
    RUNTIME_ASSERT_MSG(family, "Font family cannot be null");

    for (u32 i = 0; i < font_registry.fonts.size(); ++i) {
        const UI_Font_Info* font_info = &font_registry.fonts[i];

        if (font_info->is_loaded && strcmp(font_info->family, family) == 0 &&
            font_info->weight == weight && font_info->style == style) {
            return font_info->imgui_font;
        }
    }

    return nullptr;
}

b8 ui_fonts_rebuild() {
    if (!font_registry.is_initialized) {
        return false;
    }

    CORE_DEBUG("Rebuilding fonts..."); // Occurs infrequently; keep for diagnostics

    // Mark all fonts as not loaded
    for (u32 i = 0; i < font_registry.fonts.size(); ++i) {
        font_registry.fonts[i].is_loaded = false;
        font_registry.fonts[i].imgui_font = nullptr;
    }

    // Clear font atlas
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    // Reload all fonts
    return ui_fonts_load_all();
}

b8 ui_fonts_load_system_defaults() {
    // Register common system fonts
    // Note: This is platform-specific and would need proper implementation
    // For now, we'll just register placeholders

    b8 success = true;

    // Add default font at various sizes using constexpr values
    success &= ui_fonts_register_embedded("default_small",
        "Default",
        UI_Font_Weight::REGULAR,
        UI_Font_Style::NORMAL,
        default_font_data,
        default_font_data_size,
        UI_FONT_SIZE_SMALL);
    success &= ui_fonts_register_embedded("default_normal",
        "Default",
        UI_Font_Weight::REGULAR,
        UI_Font_Style::NORMAL,
        default_font_data,
        default_font_data_size,
        UI_FONT_SIZE_NORMAL);
    success &= ui_fonts_register_embedded("default_large",
        "Default",
        UI_Font_Weight::REGULAR,
        UI_Font_Style::NORMAL,
        default_font_data,
        default_font_data_size,
        UI_FONT_SIZE_LARGE);

    if (success) {
        CORE_INFO("System default fonts loaded");
    } else {
        CORE_ERROR("Failed to load some system default fonts");
    }

    return success;
}

// Helper function to load a single font variant at a specific size
INTERNAL_FUNC b8 load_font_variant(const char* asset_name,
    const char* family_name,
    UI_Font_Weight weight,
    UI_Font_Style style,
    f32 size) {

    u64 font_data_size;
    const u8* font_data = binary_loader_get_data(asset_name, &font_data_size);

    if (!font_data) {
        CORE_ERROR("Failed to load font asset: %s", asset_name);
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config = {};
    config.FontDataOwnedByAtlas = false;

    // Allocate font name from pool
    if (font_name_pool_index >= MAX_FONT_NAME_COUNT) {
        CORE_ERROR("Font name pool exhausted");
        return false;
    }

    char* font_name = font_name_pool[font_name_pool_index++];
    snprintf(font_name, FONT_NAME_LENGTH, "%s_%.1fpt", asset_name, size);
    strncpy(config.Name, font_name, sizeof(config.Name) - 1);

    ImFont* imgui_font = io.Fonts->AddFontFromMemoryTTF((void*)font_data,
        font_data_size,
        size,
        &config);

    if (!imgui_font) {
        CORE_ERROR("Failed to load font: %s at %.1fpt", asset_name, size);
        return false;
    }

    // Register font in registry
    UI_Font_Info font_info = {};
    font_info.name = font_name;
    font_info.family = family_name;
    font_info.weight = weight;
    font_info.style = style;
    font_info.size = size;
    font_info.data = font_data;
    font_info.data_size = font_data_size;
    font_info.imgui_font = imgui_font;
    font_info.is_loaded = true;
    font_info.is_default = false;

    font_registry.fonts.push_back(font_info);

    CORE_INFO("Font '%s' loaded (%.1fpt)", font_name, size);
    return true;
}

b8 ui_fonts_register_defaults() {
    CORE_INFO("Registering default embedded fonts...");

    // Define common sizes to load
    const f32 common_sizes[] = {
        UI_FONT_SIZE_SMALL,  // 14.0f
        UI_FONT_SIZE_NORMAL, // 17.5f
        UI_FONT_SIZE_MEDIUM, // 19.0f
        UI_FONT_SIZE_LARGE,  // 21.0f
        UI_FONT_SIZE_XLARGE  // 27.0f
    };
    const u32 size_count = sizeof(common_sizes) / sizeof(common_sizes[0]);

    b8 success = true;

    // Load Roboto family at all sizes
    for (u32 i = 0; i < size_count; ++i) {
        f32 size = common_sizes[i];

        success &= load_font_variant("roboto_regular",
            "Roboto",
            UI_Font_Weight::REGULAR,
            UI_Font_Style::NORMAL,
            size);

        success &= load_font_variant("roboto_bold",
            "Roboto",
            UI_Font_Weight::BOLD,
            UI_Font_Style::NORMAL,
            size);

        success &= load_font_variant("roboto_italic",
            "Roboto",
            UI_Font_Weight::REGULAR,
            UI_Font_Style::ITALIC,
            size);
    }

    // Load JetBrains Mono family at all sizes
    for (u32 i = 0; i < size_count; ++i) {
        f32 size = common_sizes[i];

        success &= load_font_variant("jetbrains_regular",
            "JetBrains Mono",
            UI_Font_Weight::REGULAR,
            UI_Font_Style::NORMAL,
            size);

        success &= load_font_variant("jetbrains_bold",
            "JetBrains Mono",
            UI_Font_Weight::BOLD,
            UI_Font_Style::NORMAL,
            size);

        success &= load_font_variant("jetbrains_italic",
            "JetBrains Mono",
            UI_Font_Weight::REGULAR,
            UI_Font_Style::ITALIC,
            size);
    }

    if (success) {
        CORE_INFO("Successfully loaded %d font variants",
            font_registry.fonts.size());

        // Set default font to JetBrains Mono Regular at normal size
        b8 default_set =
            ui_fonts_set_default(ui_font_config(UI_Font_Family::JETBRAINS_MONO,
                UI_Font_Weight::REGULAR,
                UI_Font_Style::NORMAL,
                UI_FONT_SIZE_MEDIUM));

        if (!default_set) {
            CORE_WARN("Failed to set default font, using ImGui default");
        }
    } else {
        CORE_WARN("Failed to load some embedded fonts");
    }

    return success;
}

// Helper function to create font config
UI_Font_Config ui_font_config(UI_Font_Family family,
    UI_Font_Weight weight,
    UI_Font_Style style,
    f32 size) {
    UI_Font_Config config;
    config.family = family;
    config.weight = weight;
    config.style = style;
    config.size = size;
    return config;
}

// Get font by name (implements declared but missing function)
ImFont* ui_fonts_get(const char* name) {
    UI_Font_Info* font_info = find_font_by_name(name);
    if (!font_info) {
        return nullptr;
    }

    if (!font_info->is_loaded) {
        CORE_WARN("Font '%s' not loaded", name);
        return nullptr;
    }

    return font_info->imgui_font;
}

// Get default font (implements declared but missing function)
ImFont* ui_fonts_get_default() { return font_registry.default_font; }

// Get font by configuration
ImFont* ui_fonts_get(UI_Font_Config config) {
    return ui_fonts_get(config.family,
        config.weight,
        config.style,
        config.size);
}

// Get font by family, weight, style, and size
ImFont* ui_fonts_get(UI_Font_Family family,
    UI_Font_Weight weight,
    UI_Font_Style style,
    f32 size) {

    // Convert family enum to string
    const char* family_str = nullptr;
    switch (family) {
    case UI_Font_Family::ROBOTO:
        family_str = "Roboto";
        break;
    case UI_Font_Family::JETBRAINS_MONO:
        family_str = "JetBrains Mono";
        break;
    default:
        CORE_ERROR("Unknown font family");
        return nullptr;
    }

    // Search for matching font
    for (u32 i = 0; i < font_registry.fonts.size(); ++i) {
        const UI_Font_Info* font_info = &font_registry.fonts[i];

        if (font_info->is_loaded &&
            strcmp(font_info->family, family_str) == 0 &&
            font_info->weight == weight && font_info->style == style &&
            font_info->size == size) {
            return font_info->imgui_font;
        }
    }

    // Font not found
    CORE_DEBUG("Font not found: %s weight=%d style=%d size=%.1f",
        family_str,
        (int)weight,
        (int)style,
        size);
    return nullptr;
}

// Set default font by configuration
b8 ui_fonts_set_default(UI_Font_Config config) {
    ImFont* font = ui_fonts_get(config);
    if (!font) {
        CORE_ERROR("Cannot set default font: font not found");
        return false;
    }

    // Update registry
    font_registry.default_font = font;

    // Update all font infos to clear default flag
    for (u32 i = 0; i < font_registry.fonts.size(); ++i) {
        font_registry.fonts[i].is_default = false;
    }

    // Set the matching font as default
    for (u32 i = 0; i < font_registry.fonts.size(); ++i) {
        UI_Font_Info* font_info = &font_registry.fonts[i];
        if (font_info->imgui_font == font) {
            font_info->is_default = true;
            break;
        }
    }

    // Set ImGui default font
    ImGuiIO& io = ImGui::GetIO();
    io.FontDefault = font;

    CORE_INFO("Set default font to: %s %.1fpt",
        config.family == UI_Font_Family::ROBOTO ? "Roboto" : "JetBrains Mono",
        config.size);
    return true;
}

// Internal function implementations

INTERNAL_FUNC UI_Font_Info* find_font_by_name(const char* name) {
    if (!name)
        return nullptr;

    // Check if font system is initialized
    if (!font_registry.is_initialized) {
        return nullptr;
    }

    // Create a temporary font info for searching
    UI_Font_Info search_font = {};
    search_font.name = name;

    // Use Auto_Array's find method
    UI_Font_Info* found = font_registry.fonts.find(search_font);

    // Check if found (find returns end() if not found)
    return (found < font_registry.fonts.end()) ? found : nullptr;
}

INTERNAL_FUNC ImFontConfig create_font_config(const UI_Font_Info* font_info) {
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false; // We manage our own font data
    config.MergeMode = false;
    config.PixelSnapH = true;
    config.GlyphMaxAdvanceX = FLT_MAX;
    config.RasterizerMultiply = 1.0f;
    config.EllipsisChar = (ImWchar)-1;

    // Set font name for debugging
    if (font_info->name) {
        strncpy(config.Name, font_info->name, sizeof(config.Name) - 1);
        config.Name[sizeof(config.Name) - 1] = '\0';
    }

    return config;
}
