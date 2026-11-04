#ifndef ENGINE_COMPONENTS_LIFETIME_HPP
#define ENGINE_COMPONENTS_LIFETIME_HPP

#include <chrono>

namespace engine::components {

struct Lifetime {
    std::chrono::steady_clock::time_point spawnTime;
    float Lifetime = 0.0f;

    float fadeStartTime = 0.0f; // Time in seconds after spawn when the entity should start fading out
    float fadeDuration = 0.0f;  // Duration in seconds of the fade out effect

    float transparencyFadingMultiplier = 1.0f; // multiplier to control the transparency fading effect
    float scaleFadingMultiplier = 1.0f;        // multiplier to control the scale fading effect
};

} // namespace engine::components

#endif // ENGINE_COMPONENTS_LIFETIME_HPP
