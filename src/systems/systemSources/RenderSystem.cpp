#include "../RenderSystem.hpp"

#include "rendering/deadzoneCamera.hpp"

#include <SFML/Graphics/RenderTarget.hpp>

#include "../../components/Render.hpp"
#include "../../components/Transform.hpp"
#include "../../core/Registry.hpp"

#include <algorithm>
#include <stdexcept>

namespace engine::systems {

void RenderSystem::render(Registry& registry, sf::RenderTarget& target, DeadZoneCamera& camera) const {
    const sf::View& view = camera.getView();
    const sf::Vector2f halfSize = view.getSize() * 0.5f;
    const sf::Vector2f viewMin = view.getCenter() - halfSize;
    const sf::Vector2f viewMax = view.getCenter() + halfSize;

    vertices.clear();
    batches.clear();
    entitiesCulled = 0;

    {
        ZoneScopedN("Render logic loop");

        registry.group<components::Transform, components::Render>().each(
            [&](Entity entity, components::Transform& transform, components::Render& r) {
                if (!r.texture) return;         // drop the cerr from the hot path

                sf::FloatRect rect = r.textureRect;
                if (rect.size.x == 0.0f && rect.size.y == 0.0f)
                    rect.size = sf::Vector2f(r.texture->getSize());   // better: cache this when the component is set

                const sf::Vector2f size = rect.size.componentWiseMul(transform.scale);
                const float rad = transform.rotation.asRadians();
                const float c = std::cos(rad), s = std::sin(rad);
                const sf::Vector2f scaledOrigin = transform.origin.componentWiseMul(transform.scale);
                const sf::Vector2f p = transform.position - sf::Vector2f(
                    c * scaledOrigin.x - s * scaledOrigin.y,
                    s * scaledOrigin.x + c * scaledOrigin.y);

                // The quad lies within a conservative square around its top-left corner.
                const float radius = std::abs(size.x) + std::abs(size.y);
                if (p.x + radius < viewMin.x || p.x - radius > viewMax.x ||
                    p.y + radius < viewMin.y || p.y - radius > viewMax.y) {
			    		entitiesCulled++;
	                    return;
				    }

				    const sf::Vector2f tr = p + sf::Vector2f(c * size.x, s * size.x);
                const sf::Vector2f bl = p + sf::Vector2f(-s * size.y, c * size.y);
                const sf::Vector2f br = tr + bl - p;

                // Exact AABB test
                const float minX = std::min({p.x, tr.x, bl.x, br.x});
                const float maxX = std::max({p.x, tr.x, bl.x, br.x});
                const float minY = std::min({p.y, tr.y, bl.y, br.y});
                const float maxY = std::max({p.y, tr.y, bl.y, br.y});
                if (maxX < viewMin.x || minX > viewMax.x || maxY < viewMin.y || minY > viewMax.y)
                    return;

                const sf::Vector2f t0(rect.position.x, rect.position.y);
                const sf::Vector2f t1(rect.position.x + rect.size.x, rect.position.y);
                const sf::Vector2f t2(rect.position.x, rect.position.y + rect.size.y);
                const sf::Vector2f t3(rect.position.x + rect.size.x, rect.position.y + rect.size.y);

                if (batches.empty() || batches.back().texture != r.texture)
                    batches.push_back({r.texture, vertices.size(), 0});

                vertices.push_back({p,  r.color, t0});
                vertices.push_back({tr, r.color, t1});
                vertices.push_back({bl, r.color, t2});
                vertices.push_back({tr, r.color, t1});
                vertices.push_back({br, r.color, t3});
                vertices.push_back({bl, r.color, t2});
                batches.back().vertexCount += 6;
            });

        if (vertices.empty())
            return;

        if (vertexBuffer.getVertexCount() < vertices.size() && !vertexBuffer.create(vertices.size()))
            throw std::runtime_error("Failed to create render system vertex buffer");

        if (!vertexBuffer.update(vertices.data(), vertices.size(), 0))
            throw std::runtime_error("Failed to update render system vertex buffer");

        for (const TextureBatch& batch : batches) {
            sf::RenderStates states;
            states.texture = batch.texture;
            target.draw(vertexBuffer, batch.firstVertex, batch.vertexCount, states);
        }
    }
}
} // namespace engine::systems