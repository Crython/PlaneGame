#include "rendering/textures.hpp"

#include <stdexcept>
#include <utility>

const std::string Textures::texturePath = std::string(PLANE_GAME_ASSET_DIR) + "/textures/";
std::unordered_map<std::string, sf::Texture> Textures::cache;

const sf::Texture& Textures::load(const std::string& name, const std::string& filename, bool repeated, bool smoothed) {
    const std::string fullPath = texturePath + filename;

    sf::Texture texture;
    if (!texture.loadFromFile(fullPath)) {
        throw std::runtime_error("Failed to load texture: " + fullPath);
    }
    texture.setRepeated(repeated);
    texture.setSmooth(smoothed);
    
    sf::Texture& stored = cache[name];
    stored = std::move(texture);
    return stored;
}

const sf::Texture& Textures::get(const std::string& name) {
    const auto it = cache.find(name);
    if (it == cache.end()) {
        throw std::runtime_error("Texture not found: " + name);
    }
    return it->second;
}