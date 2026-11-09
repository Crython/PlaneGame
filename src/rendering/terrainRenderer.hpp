#include <SFML/Graphics.hpp>
#include <vector>

#include "../core/terrainMaker.hpp"

class TerrainRenderer {
public:
    // Store a reference to the data class so we can read from it
    explicit TerrainRenderer() 
    {
        // We use Triangles to build quads (2 triangles = 6 vertices per cell)
        vao.setPrimitiveType(sf::PrimitiveType::Triangles);
    }

    // Iterates over the vector and populates the VertexArray
    void updateVAO() {
        const std::vector<std::vector<cell>>& cells = TerrainMaker::getCells();
        
        std::size_t vertexCount = 0;
        for (const auto& column : cells) {
            for (const cell& c : column) {
                if (c.type != CellType::Air) {
                    vertexCount += 6;
                }
            }
        }
        vao.resize(vertexCount);

        std::size_t vertexIndex = 0;
        
    
        for (std::size_t columnIndex = 0; columnIndex < cells.size(); ++columnIndex) {
            int runStart = TerrainMaker::heightMin;
            for (std::size_t y = 0; y < cells[columnIndex].size(); ++y) {
                const cell& c = cells[columnIndex][y];
                const int runEnd = runStart + c.verticalLength;
                const float top = -static_cast<float>(runEnd) * TerrainMaker::cellSize;
                const float bottom = -static_cast<float>(runStart) * TerrainMaker::cellSize;
                runStart = runEnd;

                if (c.type == CellType::Air) {
                    continue;
                }

                const float left = (static_cast<float>(TerrainMaker::getWorldRangeMin()) + static_cast<float>(columnIndex)) * TerrainMaker::cellSize;
                const float right = left + TerrainMaker::cellSize;

                // Calculate the 4 corners of the current terrain cell
                sf::Vector2f topLeft     = {left, top};
                sf::Vector2f topRight    = {right, top};
                sf::Vector2f bottomRight = {right, bottom};
                sf::Vector2f bottomLeft  = {left, bottom};

                // Triangle 1 (Top-Left, Top-Right, Bottom-Right)
                vao[vertexIndex + 0].position = topLeft;
                vao[vertexIndex + 1].position = topRight;
                vao[vertexIndex + 2].position = bottomRight;

                // Triangle 2 (Bottom-Right, Bottom-Left, Top-Left)
                vao[vertexIndex + 3].position = bottomRight;
                vao[vertexIndex + 4].position = bottomLeft;
                vao[vertexIndex + 5].position = topLeft;

                sf::Color color;
                switch (c.type) {
                    case CellType::Grass: color = sf::Color{47, 150, 48}; break;
                    case CellType::Dirt: color = sf::Color{122, 74, 28}; break;
                    case CellType::Stone: color = sf::Color{100, 100, 100}; break;
                    case CellType::Air: continue;
                }
                color.r = std::clamp(color.r * c.colorVariation, 0.0f, 255.0f);
                color.g = std::clamp(color.g * c.colorVariation, 0.0f, 255.0f);
                color.b = std::clamp(color.b * c.colorVariation, 0.0f, 255.0f);


                for (std::size_t i = 0; i < 6; ++i) {
                    vao[vertexIndex + i].color = color;
                }

                vertexIndex += 6;
            }
        }
    }

    void draw(sf::RenderTarget& target) const {
        sf::RenderStates states;

        // You can apply a texture or custom transform via 'states' here if needed
        target.draw(vao, states);
    }

private:
    sf::VertexArray vao; 
};