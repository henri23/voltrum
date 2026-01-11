#pragma once

#include "systems/resource_system.hpp"

/**
 * Icon Asset Loader
 * Loads icon images without vertical flipping (unlike image_loader)
 * Suitable for window icons, cursors, and other UI elements
 */

Resource_Loader icon_resource_loader_create();
