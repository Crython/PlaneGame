#ifndef TERRAIN_MAKER_HPP
#define TERRAIN_MAKER_HPP

#include "Noise.hpp"

#include <vector>
#include <iostream>

enum class CellType {
    Air,
    Grass,
    Dirt,
    Stone
};

// Uses Run-Length Encoding 
struct cell {
    //float x = 0.0f; // position is encoded in data structure
    //float y = 0.0f;
    int verticalLength = 0;
    CellType type = CellType::Air;
    float colorVariation = 0.0f;
};

class TerrainMaker {
public:
    TerrainMaker(int seed) { setSeed(seed); }
    static void setSeed(int s) { seed = s; }

    static void updateWorld(float centerX, int windowSizeX);

    static void compressHeightmap();

    static const std::vector<std::vector<cell>>& getCells() {
        return heightmap;
    }

    static const int getWorldSize() { return worldSize; }
    static const int getWorldRangeMin() { return worldRangeMin; }
    static const int getWorldRangeMax() { return worldRangeMax; }

    static inline const float cellSize = 3.0f;
    static inline const int heightMin = 0;
    static inline const int heightMax = 150;

private:
    static std::vector<std::vector<cell>> heightmap;
    static int seed;
    static int worldSize;
    static int worldRangeMin;
    static int worldRangeMax;

};

#endif // TERRAIN_MAKER_HPP