#ifndef ENGINE_SYSTEMS_PARTICLE_TRAIL_SYSTEM_HPP
#define ENGINE_SYSTEMS_PARTICLE_TRAIL_SYSTEM_HPP

#include <chrono>
#include "../systems/LifetimeSystem.hpp"


namespace engine {
class Registry;
}

namespace engine::systems {

class ParticleTrailSystem {
public:
    void update(Registry& registry, LifetimeSystem& lifetimeSystem, float deltaTime) const;

private:
    const float particleGlobalScale = 0.75f;
    mutable std::chrono::steady_clock::duration elapsed{};
};

} // namespace engine::systems

#endif // ENGINE_SYSTEMS_PARTICLE_TRAIL_SYSTEM_HPP