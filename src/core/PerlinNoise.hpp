#ifndef PERLIN_NOISE_HPP
#define PERLIN_NOISE_HPP

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

class PerlinNoise {
private:
    std::vector<int> p;

    double fade(double t) const {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    double lerp(double t, double a, double b) const {
        return a + t * (b - a);
    }

    double grad(int hash, double x, double y) const {
        int h = hash & 7;
        double u = h < 4 ? x : y;
        double v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0 * v : 2.0 * v);
    }

public:
    explicit PerlinNoise(unsigned int seed = std::random_device{}()) {
        p.resize(512);
        std::iota(p.begin(), p.begin() + 256, 0);

        std::default_random_engine engine(seed);
        std::shuffle(p.begin(), p.begin() + 256, engine);

        std::copy(p.begin(), p.begin() + 256, p.begin() + 256);
    }

    double noise(double x, double y) const {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;

        x -= std::floor(x);
        y -= std::floor(y);

        double u = fade(x);
        double v = fade(y);

        int aa = p[p[X] + Y];
        int ab = p[p[X] + Y + 1];
        int ba = p[p[X + 1] + Y];
        int bb = p[p[X + 1] + Y + 1];

        return lerp(v, lerp(u, grad(aa, x, y), grad(ba, x - 1, y)),
                    lerp(u, grad(ab, x, y - 1), grad(bb, x - 1, y - 1)));
    }
};

#endif // PERLIN_NOISE_HPP
