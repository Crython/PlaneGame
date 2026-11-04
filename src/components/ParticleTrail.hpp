#ifndef ENGINE_COMPONENTS_PARTICLE_TRAIL_HPP
#define ENGINE_COMPONENTS_PARTICLE_TRAIL_HPP

#include <chrono>

namespace engine::components {

enum class ParticleType {
    None, 
    Smoke,
    _COUNT
};

struct ParticleTrail {
    std::chrono::steady_clock::time_point lastParticleSpawnTime;
    float ParticleRate = 0.0f; // Particles per second
    ParticleType particleType = ParticleType::None;
};

} // namespace engine::components

#endif // ENGINE_COMPONENTS_PARTICLE_TRAIL_HPP
