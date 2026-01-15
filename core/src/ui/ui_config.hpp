#pragma once

// UI Configuration Options for Voltrum Engine

// Viewport Support Configuration
// ------------------------------
// SDL3 viewport support in ImGui is experimental and can cause crashes.
// Enable this only if you need multi-viewport support and are willing to test
// stability.
//
// To enable viewports, uncomment the line below:
// #define VOLTRUM_ENABLE_VIEWPORTS

// When VOLTRUM_ENABLE_VIEWPORTS is disabled:
// - Docking still works perfectly within the main window
// - No crashes related to multi-viewport rendering
// - Stable and reliable UI experience
// - Windows can be docked to each other and the main viewport

// When VOLTRUM_ENABLE_VIEWPORTS is enabled:
// - Windows can be dragged outside the main window
// - Each window becomes its own platform window
// - May cause crashes during shutdown with SDL3
// - Experimental feature - use at your own risk

#ifdef VOLTRUM_ENABLE_VIEWPORTS
#    pragma message("ImGui viewports enabled - experimental SDL3 support")
#else
#    pragma message("ImGui viewports disabled - SDL3 safe mode")
#endif

// Future UI Configuration Options
// --------------------------------
// Add more UI configuration options here as needed