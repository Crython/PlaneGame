#ifndef ENGINE_SYSTEMS_INPUT_SYSTEM_HPP
#define ENGINE_SYSTEMS_INPUT_SYSTEM_HPP

#include "SFML/Window/Keyboard.hpp"
#include "components/PlayerController.hpp"
#include "core/Entity.hpp"
#include <cstdint>
#include <array>


namespace engine {
class Registry;
}

namespace engine::systems {

class InputSystem {
public:
    void update(Registry& registry, float dt, components::AttackAction primaryAction, components::AttackAction secondaryAction);
    void handlePlaneWeapon(Registry& registry, Entity entity, float dt, int actionNum, std::uint8_t actionOrders,
                           components::AttackAction primaryAction, components::AttackAction secondaryAction);

    bool isKeyDown(sf::Keyboard::Scancode scancode) const;
    void setKeyDown(sf::Keyboard::Scancode scancode, bool down);

    void clearKeyDown();

private:
    std::array<bool, sf::Keyboard::ScancodeCount> keyDown{};

};

} // namespace engine::systems

#endif // ENGINE_SYSTEMS_INPUT_SYSTEM_HPP
