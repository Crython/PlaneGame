#ifndef PARTICLE_FACTORY_HPP
#define PARTICLE_FACTORY_HPP

#include "core/Registry.hpp"
#include "components/Components.hpp" // Include all components
#include "rendering/textures.hpp"
#include <SFML/Graphics.hpp>
#include <array>
#include <stdexcept>

#include "tracy-0.14.1/public/tracy/Tracy.hpp" // Used for accurate and easy CPU timings



class ParticleFactory {
public:
    static sf::Vector2f getRandomVariance(float maxVariance) {
        // Use a simple linear congruential generator for reproducibility
        runningVarianceSeed = (runningVarianceSeed * 1664525 + 1013904223) % 0xFFFFFFFF;
        float randomValue = static_cast<float>(runningVarianceSeed) / static_cast<float>(0xFFFFFFFF);
        return sf::Vector2f((randomValue * 2.0f - 1.0f) * maxVariance, (randomValue * 2.0f - 1.0f) * maxVariance);
    }
    static sf::Vector2f getRandomSignScalar() {
        sf::Vector2f randVal = getRandomVariance(1.0f) - sf::Vector2f(1.0f, 1.0f);

        return sf::Vector2f(randVal.x >= 0.0f ? 1.0f : -1.0f, randVal.y >= 0.0f ? 1.0f : -1.0f);
    }

    inline static sf::Vector2f Multiply(const sf::Vector2f& left, const sf::Vector2f& right) {
        return sf::Vector2f(left.x * right.x, left.y * right.y);
    }

    static const sf::Texture& getParticleTexture(engine::components::ParticleType particleType) {
        switch (particleType) {
            case engine::components::ParticleType::Smoke:
                return Textures::get("smokeParticle");
            default:
                throw std::invalid_argument("Unknown particle type");
        }
    }

    static float getParticleLifetime(engine::components::ParticleType particleType) {
        const std::size_t index = static_cast<std::size_t>(particleType);
        if (index >= particleLifetimes.size() || particleLifetimes[index] <= 0.0f) {
            throw std::invalid_argument("Unknown particle type or particle type has no lifetime");
        }
        return particleLifetimes[index];
    }

    static engine::Entity spawnParticle(engine::Registry& registry, engine::components::ParticleType particleType, const sf::Vector2f& position, const sf::Vector2f& velocity, const float sizeMult, bool addRandomVariance = true) {
        ZoneScopedN("Spawn particle");
        const sf::Texture& particleTexture = ParticleFactory::getParticleTexture(particleType);
        const float lifetime = getParticleLifetime(particleType);

        engine::Entity newParticleEntity = registry.createEntity();
        particleAmount++;

        engine::components::Render renderComponent;
        renderComponent.texture = (sf::Texture*)&particleTexture;

        auto randomVariance = [&](float maxVariance) -> sf::Vector2f {
            return (addRandomVariance ? ParticleFactory::getRandomVariance(maxVariance) : sf::Vector2f{});
        };

        engine::components::Transform transformComponent;
        transformComponent.position = position + randomVariance(5.0f);
        transformComponent.rotation = sf::degrees(randomVariance(360.0f).x); // Random rotation
        transformComponent.scale = Multiply((sf::Vector2f(1.0f, 1.0f) + randomVariance(0.5f)), addRandomVariance ? getRandomSignScalar() : sf::Vector2f(1.0f, 1.0f)) * sizeMult; // Slightly nuanced scale
        transformComponent.initialScale = transformComponent.scale;
        transformComponent.origin = sf::Vector2f(particleTexture.getSize().x / 2.0f, particleTexture.getSize().y / 2.0f); // Center the origin

        // Decrease gravity scale for particles to make them float more
        transformComponent.gravityScale = 0.02f;

        registry.addComponent(newParticleEntity, std::move(transformComponent));
	    registry.addComponent(newParticleEntity, engine::components::Kinematics{velocity});
        registry.addComponent(newParticleEntity, std::move(renderComponent));
        registry.addComponent(newParticleEntity, engine::components::Lifetime{
            std::chrono::steady_clock::now(), lifetime, 0.0f, lifetime
        });
    
        return newParticleEntity;
    }
    static void loadParticleTextures() {
        Textures::load("smokeParticle", "SmokeParticle.png", false, false);
    }

    static uint getParticleAmount() { return particleAmount; }
    static void decrementParticleAmount(int amount = 1) {
        if (particleAmount - amount < 0) {
            throw std::logic_error("Cannot decrement particle amount below zero");
        }
        particleAmount -= amount;
    }

private:
    inline static constexpr std::size_t PTC =
        static_cast<std::size_t>(engine::components::ParticleType::_COUNT);
    inline static constexpr std::array<float, PTC> particleLifetimes{{
        0.0f,
        3.5f
    }};
    inline static uint particleAmount;
    inline static uint runningVarianceSeed = 0;
};

#endif // PARTICLE_FACTORY_HPP
