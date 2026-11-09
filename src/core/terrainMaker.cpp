#include "terrainMaker.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>

std::vector<std::vector<cell>> TerrainMaker::heightmap;
int TerrainMaker::seed;
int TerrainMaker::worldSize;
int TerrainMaker::worldRangeMin;
int TerrainMaker::worldRangeMax;

void TerrainMaker::updateWorld(float centerX, int windowSizeX) {

    // fast inverse smoothstep
    auto fiSmoothstep = [](float x) -> float {
        x = std::clamp(x, 0.0f, 1.0f);
        float t = 2.0f * x - 1.0f;    
        float sign = (t > 0.0f) ? 1.0f : ((t < 0.0f) ? -1.0f : 0.0f);
        return 0.5f + 0.5f * sign * std::sqrt(std::abs(t));
    };

    const float halfWindowInCells = windowSizeX / (2.0f * cellSize);
    const int rangeMin = static_cast<int>(std::floor(centerX / cellSize - halfWindowInCells));
    const int rangeMax = static_cast<int>(std::ceil(centerX / cellSize + halfWindowInCells));

    worldSize = rangeMin + rangeMax;
    worldRangeMin = rangeMin;
    worldRangeMax = rangeMax;
    heightmap.resize(static_cast<std::size_t>(rangeMax - rangeMin));
    for (auto& column : heightmap) {
        column.resize(static_cast<std::size_t>(heightMax - heightMin));
    }

    constexpr float baseHeight = 30.0f;
    constexpr float colorDepthVariationMult = 0.015f;
    constexpr float colorNoiseVariationMult = 0.55f;
    constexpr float colorNoiseVariationFreqX = 0.09f;
    constexpr float colorNoiseVariationFreqY = 0.4f;

    constexpr float continentalFreq = 0.0017f;
    constexpr float continentalAmp  = 36.0f;
    constexpr float hillFreq = 0.006f;
    constexpr float hillAmp  = 16.0f;
    constexpr float detailFreq = 0.03f;
    constexpr float detailAmp  = 1.5f;

    Noise noise(seed);

    for (int x = rangeMin; x < rangeMax; ++x) {
        const float fx = static_cast<float>(x);
        const float h = baseHeight
                      + noise.smoothstep(static_cast<float>(noise.noisePerlin(fx * continentalFreq, 1000)) * continentalAmp + 0.5f) - 0.5f
                      + noise.smoothstep(static_cast<float>(noise.noisePerlin(fx * hillFreq, 2000))) * hillAmp
                      + static_cast<float>(noise.noisePerlin(fx * detailFreq, 3000)) * detailAmp;
        const int terrainHeight = std::clamp(static_cast<int>(std::floor(h)), heightMin, heightMax);
        auto& column = heightmap[static_cast<std::size_t>(x - rangeMin)];

        for (int y = heightMin; y < heightMax; ++y) {
            cell c;
            if (y < terrainHeight) {
                if (y == terrainHeight - 1) {
                    c.type = CellType::Grass;
                } else if (y >= terrainHeight - 8) {
                    c.type = CellType::Dirt;
                } else {
                    c.type = CellType::Stone;
                }


                int depth = terrainHeight - y;

                float colorNoise = fiSmoothstep(noise.noisePerlin(x * colorNoiseVariationFreqX, y * colorNoiseVariationFreqY)) * colorNoiseVariationMult;
                c.colorVariation = 1.0f - noise.lerp(0.0f, colorNoise, depth * colorDepthVariationMult);
            }
            column[static_cast<std::size_t>(y - heightMin)] = c;
        }
    }

    compressHeightmap();
}

void TerrainMaker::compressHeightmap() {

    std::vector<std::vector<cell>> newHeightmap(heightmap.size());

    for (std::size_t x = 0; x < heightmap.size(); ++x) {
        const auto& column = heightmap[x];
        if (column.empty()) {
            continue;
        }

        cell runningCell = column.front();
        runningCell.verticalLength = 1;

        for (std::size_t y = 1; y < column.size(); ++y) {
            if (runningCell.type == column[y].type &&
                runningCell.colorVariation == column[y].colorVariation) {
                ++runningCell.verticalLength;
            } else {
                newHeightmap[x].push_back(runningCell);
                runningCell = column[y];
                runningCell.verticalLength = 1;
            }
        }
        newHeightmap[x].push_back(runningCell);
    }
    heightmap = newHeightmap;
}