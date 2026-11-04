#ifndef ENGINE_COMPONENTS_PLAYER_PLANE_CONTROLLER_HPP
#define ENGINE_COMPONENTS_PLAYER_PLANE_CONTROLLER_HPP

#include <SFML/System/Vector2.hpp>


namespace engine::components {

enum class AttackAction {
    None,
    Bullet,
    Missile
};

struct PlayerPlaneController {
    sf::Vector2f movingDirection = {0.0f, 0.0f};
    sf::Angle rotation = sf::Angle::Zero;
    float throttle = 0;

    AttackAction primaryAction = AttackAction::None;
    AttackAction secondaryAction = AttackAction::None;
};

} // namespace engine::components

#endif // ENGINE_COMPONENTS_PLAYER_PLANE_CONTROLLER_HPP 
