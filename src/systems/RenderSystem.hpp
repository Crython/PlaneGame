#ifndef ENGINE_SYSTEMS_RENDER_SYSTEM_HPP
#define ENGINE_SYSTEMS_RENDER_SYSTEM_HPP

#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexBuffer.hpp>

#include <cstddef>
#include <vector>

namespace sf {
class RenderTarget;
class Texture;
}

namespace engine {
class Registry;
}

class DeadZoneCamera;

namespace engine::systems {

class RenderSystem {
public:
    void render(Registry& registry, sf::RenderTarget& target, DeadZoneCamera& camera) const;

    int getCulledEntityAmount() { return entitiesCulled; }
private:
    struct TextureBatch {
        const sf::Texture* texture;
        std::size_t firstVertex;
        std::size_t vertexCount;
    };

    mutable std::vector<sf::Vertex> vertices;
    mutable std::vector<TextureBatch> batches;
    mutable sf::VertexBuffer vertexBuffer{sf::PrimitiveType::Triangles, sf::VertexBuffer::Usage::Stream};
    mutable int entitiesCulled;
};

} // namespace engine::systems

#endif // ENGINE_SYSTEMS_RENDER_SYSTEM_HPP
