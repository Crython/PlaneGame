#ifndef ENGINE_COMPONENTS_TRANSFORM_HPP
#define ENGINE_COMPONENTS_TRANSFORM_HPP

#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>

namespace engine::components {

struct Transform {
    sf::Vector2f position{};
    sf::Angle rotation = sf::Angle::Zero;
    sf::Vector2f scale{1.0f, 1.0f};
    sf::Vector2f initialScale{1.0f, 1.0f}; // Scale before the fading size change
    sf::Vector2f origin{0.0f, 0.0f};
    float gravityScale = 1.0f; // Scale of gravity effect on this entity. 1.0 = normal gravity, 0.0 = no gravity, 2.0 = double gravity, etc.
};

} // namespace engine::components

#endif // ENGINE_COMPONENTS_TRANSFORM_HPP
