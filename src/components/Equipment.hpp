#ifndef ENGINE_COMPONENTS_EQUIPMENT_HPP
#define ENGINE_COMPONENTS_EQUIPMENT_HPP

#include <optional>
#include <array>
#include "core/Entity.hpp"

namespace engine::components {

struct Equipment {
    std::optional<std::array<Entity, 2>> weaponSlots;

    std::optional<std::array<Entity, 4>> armorSlots; // Currently not used

};

} // namespace engine::components

#endif // ENGINE_COMPONENTS_EQUIPMENT_HPP

