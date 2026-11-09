#ifndef NOISE_HPP
#define NOISE_HPP

#include "PerlinNoise.hpp"

#include <algorithm>
#include <cmath>

struct CliffParameters {
    float cliffThreshold = 0.5f;
    float cliffSteepness = 20.0f;
    int octaves = 4;
    float lacunarity = 2.0f;
    float gain = 0.5f;
};

class Noise {
private:
    PerlinNoise perlinNoise;

public:
    explicit Noise(unsigned int seed = std::random_device{}()) : perlinNoise(seed) {}

    double noisePerlin(double x, double y) const {
        return perlinNoise.noise(x, y);
    }

    float smoothstep(float x) { x = std::clamp(x, 0.0f, 1.0f); return x * x * (3 - 2 * x); }
    float lerp(const float& a, const float& b, const float& t) { return a + t * (b - a); }

    float noiseFBM(float x, float y, int octaves, float lacunarity, float gain) const {
        if (octaves <= 0) {
            return 0.5f;
        }

        float total = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            const float sample = static_cast<float>(noisePerlin(x * frequency, y * frequency));
            total += std::clamp((sample + 1.0f) * 0.5f, 0.0f, 1.0f) * amplitude;
            maxValue += amplitude;
            amplitude *= gain;
            frequency *= lacunarity;
        }

        return total / maxValue;
    }

    float getCliffHeight(float x, float y, const CliffParameters& params) const {
        const float baseHeight = noiseFBM(x * 0.005f, y * 0.005f, params.octaves, params.lacunarity, params.gain);
        const float cliffSelector = noiseFBM(x * 0.01f + 50.0f, y * 0.01f + 50.0f, 2, 2.0f, 0.5f);
        const float exponent = -params.cliffSteepness * (cliffSelector - params.cliffThreshold);
        const float t = std::clamp((cliffSelector - 0.4f) / 0.2f, 0.0f, 1.0f);
        const float cliffFactor = t * t * (3.0f - 2.0f * t);
        const float plateauHeight = std::floor(baseHeight * 5.0f) / 5.0f;

        return baseHeight + (plateauHeight - baseHeight) * cliffFactor;
    }
};

#endif // NOISE_HPP
