#ifndef PROJECTILE_FACTORY_HPP
#define PROJECTILE_FACTORY_HPP

#include "core/Registry.hpp"
#include "components/Components.hpp" // Include all components
#include "rendering/textures.hpp"
#include "systems/LifetimeSystem.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <array>
#include <stdexcept>
#include <chrono>

#include "tracy-0.14.1/public/tracy/Tracy.hpp" // Used for accurate and easy CPU timings

enum class ProjectileType {
    Bullet,
    Missile,
    _COUNT // Last element for array size
};

class ProjectileFactory {
public:
    static engine::Entity spawnProjectile(engine::Registry& registry, engine::systems::LifetimeSystem& lifetime, float now, engine::Entity spawnerEntity, ProjectileType projectileType) {
        ZoneScopedN("Spawn projectile");

        engine::Entity projectile = registry.createEntity();
        addComponentsToProjectile(registry, projectile, spawnerEntity, projectileType);
        lifetime.schedule(projectile, projectileLifetime[static_cast<size_t>(projectileType)].lifetime, now);
        ++projectileAmount;
        return projectile;
    }

    static void loadProjectileTextures() {
        Textures::load("bullet", "Bullet.png", false, false);
        Textures::load("missile", "Missile.png", false, false);
    }
    static float getProjectileLifetime(const ProjectileType& pt) { return projectileLifetime[static_cast<size_t>(pt)].lifetime; }
    static uint getProjectileAmount() { return projectileAmount; }
    static void decrementProjectileAmount(int amount = 1) {
        if (projectileAmount - amount < 0) {
            throw std::logic_error("Cannot decrement projectile amount below zero");
        }
        projectileAmount -= amount;
    }

private:
/*
    static engine::Entity spawnBullet(engine::Registry& registry, engine::Entity spawnerEntity) {
        return spawnProjectile(registry, spawnerEntity, ProjectileType::Bullet);
    }

    static engine::Entity spawnMissile(engine::Registry& registry, engine::Entity spawnerEntity) {
        return spawnProjectile(registry, spawnerEntity, ProjectileType::Missile);
    }
*/

    static const sf::Texture& getProjectileTexture(ProjectileType projectileType) {
        switch (projectileType) {
            case ProjectileType::Bullet:
                return Textures::get("bullet");
            case ProjectileType::Missile:
                return Textures::get("missile");
            default:
                throw std::logic_error("Invalid projectile type");
        }
    }

    static engine::components::Transform getProjectileTransform(ProjectileType projectileType, const engine::components::Transform& spawnerTransform) {

        const std::size_t index = static_cast<std::size_t>(projectileType);
        if (index >= projectileTransformOffsets.size()) {
            throw std::logic_error("Invalid projectile type");
        }

        return {
            spawnerTransform.position + projectileTransformOffsets[index].rotatedBy(spawnerTransform.rotation),
            spawnerTransform.rotation + sf::degrees(projectileRotationOffsets[index]),
            projectileScales[index],
            projectileScales[index],
            projectileOrigins[index]
        };
    }

    static engine::components::Kinematics getProjectileKinematics(ProjectileType projectileType, const engine::components::Transform& projectileTransform, sf::Vector2f spawnerVelocity) {

        const std::size_t index = static_cast<std::size_t>(projectileType);
        if (index >= projectileVelocities.size()) {
            throw std::logic_error("Invalid projectile type");
        }

        return {
            projectileVelocities[index].rotatedBy(projectileTransform.rotation) + spawnerVelocity, // Projectiles inherit their owner's velocity
            projectileAcceleration[index].rotatedBy(projectileTransform.rotation),
            projectileJerk[index].rotatedBy(projectileTransform.rotation)
        };
    }

    static void addComponentsToProjectile(engine::Registry& registry, engine::Entity projectile, engine::Entity spawnerEntity, ProjectileType projectileType) {

        const auto& spawnerTransform = registry.getComponent<engine::components::Transform>(spawnerEntity);
        const auto& spawnerKinematics = registry.getComponent<engine::components::Kinematics>(spawnerEntity);
        const engine::components::Transform transform = getProjectileTransform(projectileType, spawnerTransform);
        const engine::components::Kinematics kinematics = getProjectileKinematics(projectileType, transform, spawnerKinematics.velocity);
        const auto& particleTrail = projectileParticleTrails[static_cast<std::size_t>(projectileType)];


        registry.addComponent(projectile, transform);
        registry.addComponent(projectile, kinematics);
        registry.addComponent(projectile, engine::components::Render{const_cast<sf::Texture*>(&getProjectileTexture(projectileType))});
        registry.addComponent(projectile, engine::components::Lifetime{std::chrono::steady_clock::time_point{}, projectileLifetime[static_cast<std::size_t>(projectileType)].lifetime, projectileLifetime[static_cast<std::size_t>(projectileType)].fadeStartTime, projectileLifetime[static_cast<std::size_t>(projectileType)].fadeDuration, projectileLifetime[static_cast<std::size_t>(projectileType)].transparencyFadingMultiplier, projectileLifetime[static_cast<std::size_t>(projectileType)].scaleFadingMultiplier});
        registry.addComponent(projectile, engine::components::ParticleTrail{std::chrono::steady_clock::time_point{}, particleTrail.particleRate, particleTrail.particleType});

    }


    struct ProjectileParticleTrailConfig {
        float particleRate;
        engine::components::ParticleType particleType;
    };
    struct LifetimeConfig {
        std::chrono::steady_clock::time_point spawnTime;
        float lifetime;

        float fadeStartTime = 0.0f;
        float fadeDuration = 0.0f;

        float transparencyFadingMultiplier = 1.0f;
        float scaleFadingMultiplier = 1.0f;
    };


    inline static uint projectileAmount;
    inline static constexpr size_t PTC = static_cast<std::size_t>(ProjectileType::_COUNT);

    // Transform
    inline static constexpr std::array<sf::Vector2f, PTC> projectileTransformOffsets{{
        {46.0f, 22.0f},
        {13.0f, 20.0f}
    }};
    inline static constexpr std::array<float, PTC> projectileRotationOffsets{{ // Muzzle up rotation (-rot = up)
        -4.0f,
        -2.0f
    }};
    inline static constexpr std::array<sf::Vector2f, PTC> projectileScales{{
        {0.35f, 0.35f},
        {0.4f, 0.4f}
    }};
    inline static constexpr std::array<sf::Vector2f, PTC> projectileOrigins{{ // Texture origin
        {9.5f, 9.5f},
        {33.0f, 14.0f}
    }};

    // Kinematics
    inline static constexpr std::array<sf::Vector2f, PTC> projectileVelocities{{
        {425.0f, 0.0f},
        {0.0f, 70.0f}
    }};
    inline static constexpr std::array<sf::Vector2f, PTC> projectileAcceleration{{
        {0.0f, 0.0f},
        {-5.0f, -70.0f}
    }};
    inline static constexpr std::array<sf::Vector2f, PTC> projectileJerk{{
        {0.0f, 0.0f},
        {900.0f, 0.0f}
    }};

    // Lifetime
    inline static constexpr std::array<LifetimeConfig, PTC> projectileLifetime{{
        // spawnTime, lifetime, fadeStartTime, fadeDuration, transparencyFadingMultiplier, scaleFadingMultiplier
        {{}, 5.0f, 3.0f, 2.0f, 1.0f, 1.0f},
        {{}, 5.0f, 3.0f, 2.0f, 1.0f, 1.0f}
    }};

    // Particle trails
    inline static constexpr std::array<ProjectileParticleTrailConfig, PTC> projectileParticleTrails{{
        {0.0f, engine::components::ParticleType::None},
        {100.0f, engine::components::ParticleType::Smoke}
    }};
};

#endif // PROJECTILE_FACTORY_HPP