#include "../InputSystem.hpp"//

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "../../components/Components.hpp"
#include "../../core/Registry.hpp"

#include "tracy-0.14.1/public/tracy/Tracy.hpp" // Used for accurate and easy CPU timings

namespace engine::systems {

void InputSystem::update(Registry& registry, float dt, components::AttackAction primaryAction, components::AttackAction secondaryAction) {
    ZoneScopedN("Input");
    if (!std::isfinite(dt) || dt < 0.0f) {
        throw std::invalid_argument("InputSystem::update requires finite, non-negative delta time");
    }

    using Key = sf::Keyboard::Scancode;

    {
        ZoneScopedN("Input logic loop");

        // Several types: walks the smallest pool (PlayerPlaneController, normally one entity)
        // and skips anything missing Transform or Kinematics.
        registry.each<components::PlayerPlaneController, components::Transform, components::Kinematics>(
            [&](Entity entity, components::PlayerPlaneController& PPC, components::Transform&, components::Kinematics&) {
            PPC.primaryAction = engine::components::AttackAction::None;
            PPC.secondaryAction = engine::components::AttackAction::None;

            // Get all of the inputs
            const bool throttleOn  = isKeyDown(Key::W) || isKeyDown(Key::Up);
            const bool throttleOff = isKeyDown(Key::S) || isKeyDown(Key::Down);
            const bool turnLeft    = isKeyDown(Key::A) || isKeyDown(Key::Left);
            const bool turnRight   = isKeyDown(Key::D) || isKeyDown(Key::Right);

            uint8_t actionOrders = 0;
            actionOrders |= isKeyDown(Key::Space) << 0; // Primary action
            actionOrders |= isKeyDown(Key::X) << 1;     // Secondary action


            PPC.throttle = (throttleOn ? 1.0f : 0.0f) - (throttleOff ? 0.2f : 0.0f); // Sets throttle to {-0.2, 0.0, 0.8, 1.0}

            // Rotation is a per-frame delta; positive is clockwise in SFML's coordinate system.
            const float turn = (turnRight ? 1.0f : 0.0f) - (turnLeft ? 1.0f : 0.0f);
            constexpr float kTurnRate = 90.0f;
            PPC.rotation = sf::degrees(turn * kTurnRate * dt);

            if (registry.hasComponent<engine::components::Equipment>(entity)) {
                const auto& equipment = registry.getComponent<engine::components::Equipment>(entity);
                if (equipment.weaponSlots) {
                    handlePlaneWeapon(registry, entity, dt, 0, actionOrders, primaryAction, secondaryAction);
                    handlePlaneWeapon(registry, entity, dt, 1, actionOrders, primaryAction, secondaryAction);
                }
            }
        });
    }
}

void InputSystem::handlePlaneWeapon(Registry& registry, Entity entity, float dt, int actionNum, std::uint8_t actionOrders,
                                    components::AttackAction primaryAction, components::AttackAction secondaryAction) {
    if (actionNum < 0 || static_cast<std::size_t>(actionNum) >= 2) {
        throw std::out_of_range("Weapon action index is out of range");
    }
    if (!std::isfinite(dt) || dt < 0.0f) {
        throw std::invalid_argument("Weapon update requires finite, non-negative delta time");
    }

    const auto shouldOrderAction = [actionNum, actionOrders]() -> bool {
        return ((actionOrders >> actionNum) & 1U) != 0;
    };
    auto& playerEquipment = registry.getComponent<engine::components::Equipment>(entity);

    if (!playerEquipment.weaponSlots) {
        return;
    }
    const auto& weaponEntity = playerEquipment.weaponSlots->at(actionNum);
    if (!registry.isAlive(weaponEntity) || !registry.hasComponent<engine::components::Weapon>(weaponEntity)) {
        return;
    }
    auto& weapon = registry.getComponent<engine::components::Weapon>(weaponEntity);

    if (!std::isfinite(weapon.fireRate) || weapon.fireRate <= 0.0f) {
        throw std::logic_error("Weapon fire rate must be finite and positive");
    }
    if (!std::isfinite(weapon.currentCooldown)) {
        throw std::logic_error("Weapon cooldown must be finite");
    }

    const double cooldown = static_cast<double>(weapon.currentCooldown) - dt;
    if (cooldown <= 0.0) {
        auto& playerPlaneController = registry.getComponent<engine::components::PlayerPlaneController>(entity);
        if (shouldOrderAction()) {
            if (actionNum == 0) playerPlaneController.primaryAction = primaryAction;
            if (actionNum == 1) playerPlaneController.secondaryAction = secondaryAction;
        }

        const double fireRate = weapon.fireRate;
        const double overdue = -cooldown;
        const double remainder = std::fmod(overdue, fireRate);
        weapon.currentCooldown = static_cast<float>(remainder == 0.0 ? fireRate : fireRate - remainder);
    } else {
        weapon.currentCooldown = static_cast<float>(cooldown);
    }
}


bool InputSystem::isKeyDown(sf::Keyboard::Scancode scancode) const {
    const int index = static_cast<int>(scancode);   // Scancode::Unknown is -1
    return index >= 0 && static_cast<std::size_t>(index) < keyDown.size() && keyDown[index];
}

void InputSystem::setKeyDown(sf::Keyboard::Scancode scancode, bool down) {
    const int index = static_cast<int>(scancode);
    if (index >= 0 && static_cast<std::size_t>(index) < keyDown.size()) keyDown[index] = down;
}

void InputSystem::clearKeyDown() {
    keyDown = {};
}

} // namespace engine::systems