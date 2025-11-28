#pragma once

#include "systems/resource_system.hpp"

// Get raw binary data by name
const u8* binary_loader_get_data(const char* asset_name, u64* out_size);

Resource_Loader binary_resource_loader_create();

