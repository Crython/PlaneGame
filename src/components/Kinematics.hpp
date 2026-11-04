#ifndef ENGINE_COMPONENTS_KINEMATICS_HPP
#define ENGINE_COMPONENTS_KINEMATICS_HPP

#include <SFML/System/Vector2.hpp>

namespace engine::components {

struct Kinematics {
    sf::Vector2f velocity{}; // Describes how fast and in what direction something moves.
    sf::Vector2f acceleration{}; // Describes how quickly an object's speed or direction changes.
    sf::Vector2f jerk{}; // Describes how smoothly or abruptly an object's acceleration changes.
};

} // namespace engine::components

#endif // ENGINE_COMPONENTS_KINEMATICS_HPP
