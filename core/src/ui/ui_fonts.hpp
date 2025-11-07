#pragma once

#include "defines.hpp"

// Forward declarations
struct ImFont;
struct ImFontConfig;

// Common font sizes
constexpr f32 UI_FONT_SIZE_SMALL = 14.0f;
constexpr f32 UI_FONT_SIZE_NORMAL = 17.0f;
constexpr f32 UI_FONT_SIZE_MEDIUM = 19.0f;
constexpr f32 UI_FONT_SIZE_LARGE = 21.0f;
constexpr f32 UI_FONT_SIZE_XLARGE = 27.0f;

/**
 * UI Font Management System
 * Provides font loading, registration, and management for the UI
 * Supports embedded fonts and dynamic font scaling
 */

// Font family enumeration
enum class UI_Font_Family { ROBOTO, JETBRAINS_MONO };

// Font weight and style enumeration
enum class UI_Font_Weight {
    LIGHT = 100,
    REGULAR = 400,
    MEDIUM = 500,
    BOLD = 700
};

enum class UI_Font_Style { NORMAL, ITALIC, OBLIQUE };

// Font configuration structure
struct UI_Font_Config {
    UI_Font_Family family;
    UI_Font_Weight weight;
    UI_Font_Style style;
    f32 size;
};

/**
 * Initialize the font management system
 * @return true if successful, false otherwise
 */
VOLTRUM_API b8 ui_fonts_initialize();

/**
 * Register an embedded font
 * @param name - unique font identifier
 * @param family - font family name
 * @param weight - font weight
 * @param style - font style
 * @param data - embedded font data
 * @param data_size - size of font data in bytes
 * @param size - font size
 * @return true if successful, false otherwise
 */
VOLTRUM_API b8 ui_fonts_register_embedded(const char* name,
    const char* family,
    UI_Font_Weight weight,
    UI_Font_Style style,
    const u8* data,
    u32 data_size,
    f32 size);

/**
 * Register a system font (for future use)
 * @param name - unique font identifier
 * @param family - font family name
 * @param weight - font weight
 * @param style - font style
 * @param filepath - path to font file
 * @param size - font size
 * @return true if successful, false otherwise
 */
VOLTRUM_API b8 ui_fonts_register_system(const char* name,
    const char* family,
    UI_Font_Weight weight,
    UI_Font_Style style,
    const char* filepath,
    f32 size);

/**
 * Load all registered fonts into ImGui
 * Must be called after ImGui context is created
 * @return true if successful, false otherwise
 */
VOLTRUM_API b8 ui_fonts_load_all();

/**
 * Set the default font by name
 * @param name - name of registered font to use as default
 * @return true if successful, false otherwise
 */
VOLTRUM_API b8 ui_fonts_set_default(const char* name);

/**
 * Get a font by name
 * @param name - name of font to retrieve
 * @return pointer to ImFont or nullptr if not found
 */
VOLTRUM_API ImFont* ui_fonts_get(const char* name);

/**
 * Get the default font
 * @return pointer to default ImFont
 */
VOLTRUM_API ImFont* ui_fonts_get_default();

/**
 * Get font by configuration
 * @param config - font configuration specifying family, weight, style, and size
 * @return pointer to ImFont or nullptr if not found
 */
VOLTRUM_API ImFont* ui_fonts_get(UI_Font_Config config);

/**
 * Get font by family, weight, style, and size
 * @param family - font family
 * @param weight - font weight
 * @param style - font style
 * @param size - font size in points
 * @return pointer to ImFont or nullptr if not found
 */
VOLTRUM_API ImFont* ui_fonts_get(UI_Font_Family family,
    UI_Font_Weight weight,
    UI_Font_Style style,
    f32 size);

/**
 * Set the default font by configuration
 * @param config - font configuration
 * @return true if successful, false otherwise
 */
VOLTRUM_API b8 ui_fonts_set_default(UI_Font_Config config);

/**
 * Find font by family and style
 * @param family - font family name
 * @param weight - font weight
 * @param style - font style
 * @return pointer to ImFont or nullptr if not found
 */
VOLTRUM_API ImFont* ui_fonts_find_by_style(const char* family,
    UI_Font_Weight weight,
    UI_Font_Style style);

/**
 * Rebuild fonts (useful for reloading after changes)
 * @return true if successful, false otherwise
 */
VOLTRUM_API b8 ui_fonts_rebuild();

// Built-in font helpers
/**
 * Load default system fonts
 * Loads commonly available system fonts as fallbacks
 * @return true if successful, false otherwise
 */
VOLTRUM_API b8 ui_fonts_load_system_defaults();

/**
 * Register default embedded fonts
 * Should include basic UI fonts embedded in the application
 * @return true if successful, false otherwise
 */
VOLTRUM_API b8 ui_fonts_register_defaults();

// Font size helpers

/**
 * Helper to create a font configuration
 * @param family - font family
 * @param weight - font weight (default: REGULAR)
 * @param style - font style (default: NORMAL)
 * @param size - font size in points (default: UI_FONT_SIZE_NORMAL)
 * @return UI_Font_Config structure
 */
VOLTRUM_API UI_Font_Config ui_font_config(UI_Font_Family family,
    UI_Font_Weight weight = UI_Font_Weight::REGULAR,
    UI_Font_Style style = UI_Font_Style::NORMAL,
    f32 size = UI_FONT_SIZE_NORMAL);
