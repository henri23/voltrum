#include "systems/texture_system.hpp"

namespace systems {

b8 texture_system_init(const Texture_System_Config& /*config*/) {
    // TODO: initialize texture resources/cache here
    return true;
}

void texture_system_shutdown() {
    // TODO: release texture resources/cache here
}

} // namespace systems
