#ifndef ENGINE_COMPONENTS_RENDER_HPP
#define ENGINE_COMPONENTS_RENDER_HPP

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>

namespace sf {
class Texture;
}

#include <cstdlib>

namespace engine::components {

struct Render {
    sf::Texture* texture = nullptr;
    sf::FloatRect textureRect; // E.g., {0, 0, 32, 32}
    sf::Color color = sf::Color::White; // RGBA
    uint8_t initialAlpha = 255;
};


} // namespace engine::components

#endif // ENGINE_COMPONENTS_RENDER_HPP
