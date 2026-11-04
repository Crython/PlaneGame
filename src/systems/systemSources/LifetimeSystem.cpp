#include "../LifetimeSystem.hpp"
#include "factories/ProjectileFactory.hpp"
#include "factories/ParticleFactory.hpp"

#include <algorithm>
#include <stdexcept>
#include <iostream>

#include "tracy-0.14.1/public/tracy/Tracy.hpp" // Used for accurate and easy CPU timings

namespace engine::systems {

void LifetimeSystem::update(Registry& registry, float deltaTime) const {
    ZoneScopedN("Lifetime");

    auto isExpired = [&](Entity entity) -> bool {
        for (Entity e : expiredEntities) {
            if (entity == e) return true;
        }
        return false;
    };

    // The whole project uses steady_clock, so dt is. guaranteed to be >= 0
    // if (deltaTime < 0.0f) throw std::invalid_argument("Lifetime delta time cannot be negative");

    now += deltaTime;
    int projectilesDestroyed = 0;
    int particlesDestroyed = 0;


    // Iterate through the expiered entities
    while (!expiryQueue.empty() && expiryQueue.top().time <= now) {
        Entity entity = expiryQueue.top().entity;
        expiryQueue.pop();
        
        if (!registry.isAlive(entity) || isExpired(entity)) continue;
        if (registry.hasComponent<components::ParticleTrail>(entity)) {
            projectilesDestroyed++;
        }
        else {
            particlesDestroyed++;
        }
        expiredEntities.push_back(entity);
    }

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    registry.group<components::Lifetime, components::Render, components::Transform>().each([&](Entity entity, components::Lifetime& lifetime, components::Render& render, components::Transform& transform) {
        if (isExpired(entity) ||
            lifetime.transparencyFadingMultiplier <= 0.0f ||
            lifetime.scaleFadingMultiplier <= 0.0f ||
            lifetime.fadeDuration <= 0.0f) {
            return;
        }


        const float elapsedSinceSpawn = std::chrono::duration<float>(now - lifetime.spawnTime).count();
        const float fadeEndTime = lifetime.fadeStartTime + lifetime.fadeDuration;
        if (elapsedSinceSpawn < lifetime.fadeStartTime || elapsedSinceSpawn >= fadeEndTime) return;

        const float fadeProgress = (elapsedSinceSpawn - lifetime.fadeStartTime) / lifetime.fadeDuration;

        // Fade the transparency
        render.color.a = render.initialAlpha * (1.0f - fadeProgress * lifetime.transparencyFadingMultiplier);
        transform.scale = transform.initialScale * (1.0f - fadeProgress * lifetime.scaleFadingMultiplier);

    });

    // Destroy the expired entities
    for (Entity entity : expiredEntities) {
        registry.destroyEntity(entity); // registry checks if entity is alive before destroying
    }
    expiredEntities.clear();

    ProjectileFactory::decrementProjectileAmount(projectilesDestroyed);
    ParticleFactory::decrementParticleAmount(particlesDestroyed);

}

} // namespace engine::systems