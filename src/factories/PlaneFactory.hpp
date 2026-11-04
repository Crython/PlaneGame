#ifndef PLANE_FACTORY_HPP
#define PLANE_FACTORY_HPP

#include "core/Registry.hpp"
#include "components/Components.hpp" // Include all components
#include "rendering/textures.hpp"
#include <SFML/Graphics.hpp>
#include <stdexcept>

#include "tracy-0.14.1/public/tracy/Tracy.hpp" // Used for accurate and easy CPU timings

enum class PlaneType {
    Basic
};

class PlaneFactory {
public:
    static engine::Entity spawnPlane(engine::Registry& registry) {
        ZoneScopedN("Spawn plane");
        const sf::Texture& planeTexture = Textures::get("plane");
    
        engine::Entity newPlaneEntity = registry.createEntity();
        planeAmount++;

        engine::components::Render renderComponent;
        renderComponent.texture = (sf::Texture*)&planeTexture;

        registry.addComponent(newPlaneEntity, engine::components::Transform{{0, -300}});
	    registry.addComponent(newPlaneEntity, engine::components::Kinematics{sf::Vector2f{130.f, 0.f}});
        registry.addComponent(newPlaneEntity, std::move(renderComponent));
        registry.addComponent(newPlaneEntity, engine::components::PlayerPlaneController{});
        registry.addComponent(newPlaneEntity, engine::components::Equipment{});
    
        return newPlaneEntity;
    }
    static void loadPlaneTextures() {
        Textures::load("plane", "Plane.png", false, false);
    }

    static uint getPlaneAmount() { return planeAmount; }
    static void decrementPlaneAmount() {
        if (planeAmount == 0) {
            throw std::logic_error("Cannot decrement plane amount below zero");
        }
        --planeAmount;
    }

private:
    inline static uint planeAmount;

};

#endif // PLANE_FACTORY_HPP