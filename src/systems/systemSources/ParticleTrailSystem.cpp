#include "../ParticleTrailSystem.hpp"

#include "../../components/Kinematics.hpp"
#include "../../components/ParticleTrail.hpp"
#include "../../components/Transform.hpp"
#include "../../core/Registry.hpp"
#include "../../factories/ParticleFactory.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "tracy-0.14.1/public/tracy/Tracy.hpp"

namespace engine::systems {

void ParticleTrailSystem::update(Registry& registry, LifetimeSystem& lifetimeSystem, float deltaTime) const {
    ZoneScopedN("Particle trail");

    if (!std::isfinite(deltaTime) || deltaTime < 0.0f) {
        throw std::invalid_argument("Particle trail delta time must be finite and non-negative");
    }

    elapsed += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<float>(deltaTime));
    const auto now = std::chrono::steady_clock::time_point(elapsed);

    struct SpawnRequest {
        components::ParticleType type;
        sf::Vector2f position;
        sf::Vector2f velocity;
    };
    std::vector<SpawnRequest> spawnRequests;

    registry.group<components::ParticleTrail, components::Transform, components::Kinematics>().each(
        [&](Entity, components::ParticleTrail& trail, components::Transform& transform,
            components::Kinematics& kinematics) {
            if (trail.particleType == components::ParticleType::None ||
                !std::isfinite(trail.ParticleRate) || trail.ParticleRate <= 0.0f) {
                return;
            }

            if (trail.lastParticleSpawnTime == std::chrono::steady_clock::time_point{}) {
                trail.lastParticleSpawnTime = now;
                return;
            }

            const float intervalSeconds = 1.0f / trail.ParticleRate;
            if (!std::isfinite(intervalSeconds)) {
                return;
            }
            const auto interval = std::max(
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<float>(intervalSeconds)),
                std::chrono::steady_clock::duration{1});

            while (now - trail.lastParticleSpawnTime >= interval) {
                spawnRequests.push_back({trail.particleType, transform.position, kinematics.velocity});
                trail.lastParticleSpawnTime += interval;
            }
        });

    for (const SpawnRequest& request : spawnRequests) {
        engine::Entity entity = ParticleFactory::spawnParticle(registry, request.type, request.position, {0.0f, 0.0f}, particleGlobalScale, true);
        lifetimeSystem.schedule(entity, ParticleFactory::getParticleLifetime(request.type), std::chrono::duration_cast<std::chrono::duration<float>>(elapsed).count());
    }
}

} // namespace engine::systems
