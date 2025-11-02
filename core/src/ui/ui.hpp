#pragma once

#include "containers/auto_array.hpp"
#include "events/events.hpp"
#include "ui_themes.hpp"
#include "ui_types.hpp"

// Forward declarations
struct ImDrawData;

/**
 * Initialize the UI subsystem
 * @param theme - UI theme to use
 * @param layers - UI layer array from client
 * @param menu_callback - callback for rendering menus
 * @param app_name - application name for titlebar
 * @param window - SDL window for ImGui initialization
 * @return true if successful, false otherwise
 */
VOLTRUM_API b8 ui_initialize(
    UI_Theme theme,
	Auto_Array<UI_Layer>* layers,
	PFN_menu_callback menu_callback,
    const char* app_name,
    void* window);

/**
 * Shutdown the UI subsystem
 */
VOLTRUM_API void ui_shutdown();

/**
 * Register a UI component with the system
 * @param component - component to register (copied internally)
 * @return true if successful, false otherwise
 */
VOLTRUM_API b8 ui_register_component(const UI_Layer* component);

// Internal functions for core components only
/**
 * Get the current UI theme (internal use only)
 * @return current UI theme
 */
UI_Theme ui_get_current_theme();

/**
 * Set the UI theme at runtime
 * @param theme - new theme to apply
 */
VOLTRUM_API void ui_set_theme(UI_Theme theme);

/**
 * Get the UI event callback for registration by application
 * @return PFN_event_callback for UI event handling
 */
VOLTRUM_API PFN_event_callback ui_get_event_callback();

/**
 * Get the ImGui context for Windows DLL compatibility
 * This is needed on Windows when ImGui is built into a DLL
 * The client must call ImGui::SetCurrentContext() with this value
 * @return void* - ImGui context pointer (cast to ImGuiContext*)
 */
VOLTRUM_API void* ui_get_imgui_context();
