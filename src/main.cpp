#include "game/main.hpp"

#include <algorithm>
#include <cstdlib>
#include <exception> 
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "tracy-0.14.1/public/tracy/Tracy.hpp" // Used for accurate and easy CPU timings

namespace {

constexpr double kFramerateLimit = 60.0;
constexpr float kMaxFrameTime = 0.1f;    // seconds. Caps dt so a stall (window drag, breakpoint) can't wreck the physics
constexpr float kTurnRate = 75.0f;       // degrees per second while a turn key is held

// The camera follows the plane exactly. Raise this to let the plane drift inside a box first.
const sf::Vector2f kCameraDeadZone(0.0f, 0.0f);

// The plane starts centred horizontally, 30% of the window height above the ground.
sf::Vector2f planeStartPosition(const WindowParameters& params, const World& world) {
    return {params.width * 0.5f, world.groundLevel - params.height * 0.3f};
}

float reportInterval = 0;
int precision = 0;
double FPS = 0;
void setPrecision(int p) {
        precision = p;
    }
void setReportInterval(float rI) {
    reportInterval = rI;
}
// Prints the frame rate to stdout 'reportInterval' times a second.
class FpsCounter {
public:
    void tick() {
        ++frames;
        const float elapsed = clock.getElapsedTime().asSeconds();
        if (elapsed >= reportInterval) {
            FPS = frames / elapsed;

            frames = 0;
            clock.restart();
        }
    }

private:
    sf::Clock clock;
    int frames = 0;
};

} // namespace

Game::Game()
    : windowParams{},
      world{},
      registry(),
      window(sf::VideoMode(windowParams.getSizeU()), windowParams.title),
      camera(windowParams.getSizeF(), kCameraDeadZone),
      font(std::filesystem::path(PLANE_GAME_ASSET_DIR) / "fonts/Geneva.ttf"),
      text(font, "", 12)
{
    text.setFillColor(sf::Color::Black);
    text.setStyle(sf::Text::Bold);
    text.setPosition({10.0f, 10.0f});


    loadBackground();
    setUpVerticalLayout();
	background.setMaxZoom(1.15f);
}

// Tells the camera and the background how far the view can move up and down.
void Game::setUpVerticalLayout() {
    const float viewHeight = windowParams.getSizeF().y;

    // The view never goes below the bottom of the world. This is deliberately NOT the ground level:
    // moving the ground must not change how far the camera or the background layers travel.
    const float lowestViewBottomY = World::bottom;

    // The view's top edge is highest when the camera is centred on the plane at the ceiling.
    const float highestViewTopY = world.ceiling - viewHeight * 0.5f;

    camera.setVerticalLimits(highestViewTopY, lowestViewBottomY);

    const float maxViewRise = lowestViewBottomY - (highestViewTopY + viewHeight);
    background.setVerticalRange(lowestViewBottomY, maxViewRise);
}

void Game::loadBackground() {
    // Listed back to front: the first entry is drawn first (furthest away).
    // Fields: file, scroll speed (0 = glued to the view, 1 = fixed in the world), vertical factor
    // (multiplies the scroll speed for vertical movement; defaults to 1).
    struct LayerDef {
        const char* file;
    	float scrollSpeed;
    	float verticalFactor = 1.0f;
    	bool coversView = false;
	};

    static const LayerDef layers[] = {
        {"backgrounds/bg1/00_background.png",     0.01f, 1.0f, true},
        {"backgrounds/bg1/01_back clouds.png",    0.05f, 1.0f, true},
        {"backgrounds/bg1/02_cloud1.png",         0.05f, 1.4f},
        {"backgrounds/bg1/03_cloud2.png",         0.05f, 1.5f},
        {"backgrounds/bg1/04_cloud3.png",         0.05f, 1.6f},
        {"backgrounds/bg1/05_cloud4.png",         0.05f, 1.7f},
        {"backgrounds/bg1/06_back_mountains.png", 0.10f, 1.3f},
        {"backgrounds/bg1/07_mid_hills.png",      0.16f, 1.25f},
        //{"backgrounds/bg1/08_mid_forest.png",     0.25f},
        //{"backgrounds/bg1/09_front_grass.png",    0.67f},
        //{"backgrounds/bg1/10_front_leafs.png",    0.68f},
        // {"backgrounds/bg1/11_front_trees1.png", 0.69f},
        // {"backgrounds/bg1/12_front_trees2.png", 0.70f},
        // {"backgrounds/bg1/13_front_rocks.png",  0.71f},
    };

    for (const LayerDef& layer : layers) {
    	background.addLayer(layer.file, layer.scrollSpeed, layer.verticalFactor, layer.coversView);
    }
}


// Handles only window closing and setting the keydown arrays
void Game::handleEvents() {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            inputSystem.setKeyDown(key->scancode, true);
        } else if (const auto* key = event->getIf<sf::Event::KeyReleased>()) {
            inputSystem.setKeyDown(key->scancode, false);
        } else if (event->is<sf::Event::FocusLost>()) {
            inputSystem.clearKeyDown(); // we won't see the key-up events, so don't leave keys stuck down
        }
    }
}



void Game::handleEntityAttackAction(engine::Entity entity, engine::components::AttackAction attackAction) {
    switch (attackAction) {
        case engine::components::AttackAction::Bullet:
            shootProjectile(entity, ProjectileType::Bullet, 7.0f);
            break;
        case engine::components::AttackAction::Missile:
            shootProjectile(entity, ProjectileType::Missile, 0.0f);
            break;
        case engine::components::AttackAction::None:
            break;
        default:
            throw std::logic_error("Attack action not implemented or invalid!\n");
    }
}


// Call this function once to initialze every component that any entity might need
void Game::InitializeAllEntityComponents() {
    initializePlaneComponents();
    initializeProjectileComponents();
    initializeWeaponComponents(false); // Initialize all
}

void Game::initializePlaneComponents() {
    // Initialize the components
    // If some of the components are already created, then the registry will ignore them
    registry.registerComponent<engine::components::Transform>();
    registry.registerComponent<engine::components::Kinematics>();
    registry.registerComponent<engine::components::Render>();
    registry.registerComponent<engine::components::PlayerPlaneController>();
    registry.registerComponent<engine::components::Equipment>();
}
void Game::initializeProjectileComponents() {
    // Initialize the components
    // If some of the components are already created, then the registry will ignore them
    registry.registerComponent<engine::components::Transform>();
    registry.registerComponent<engine::components::Kinematics>();
    registry.registerComponent<engine::components::Render>();
    registry.registerComponent<engine::components::Lifetime>();

    // Register the ParticleTrail component. TODO: Move this to it's own initializeParticleComponents() function incase more particle components are added in the future
    registry.registerComponent<engine::components::ParticleTrail>();
}
void Game::initializeWeaponComponents(bool weaponHidden) {
    // Initialize the components
    // If some of the components are already created, then the registry will ignore them
    if (!weaponHidden) { // Hidden weapons don't need these components
        registry.registerComponent<engine::components::Transform>();
        registry.registerComponent<engine::components::Kinematics>();
        registry.registerComponent<engine::components::Render>();
    }

    registry.registerComponent<engine::components::Weapon>();
}

void Game::initializeFactoryTextures() {
    ProjectileFactory::loadProjectileTextures();
    PlaneFactory::loadPlaneTextures();
    ParticleFactory::loadParticleTextures();
}

// Add a weapon of type Entity to another entity with equipment
void Game::addWeaponToEntity(engine::Entity entity, int weaponSlot, engine::Entity weapon) {
    if (!registry.hasComponent<engine::components::Equipment>(entity)) {
        throw std::runtime_error("Target entity doesn't have an equipment component\n");
    }

    auto& equipment = registry.getComponent<engine::components::Equipment>(entity);
    if (!equipment.weaponSlots) {
        equipment.weaponSlots.emplace();
    }
    equipment.weaponSlots->at(weaponSlot) = weapon;
}

void Game::addWeaponsToEntity(engine::Entity entity) {
    if (!registry.isAlive(entity)) {
        throw std::runtime_error("Cannot add weapons to non-existing entity " + std::to_string(entity.id));
    }

    engine::Entity primaryWeapon = registry.createEntity();
    engine::components::Weapon pWeapon;
    pWeapon.fireRate = 1 / 25.0f; 
    pWeapon.currentCooldown = pWeapon.fireRate;
    pWeapon.damage = 3.5f; 

    registry.addComponent(primaryWeapon, pWeapon);
    addWeaponToEntity(entity, 0, primaryWeapon); // Add the weapon component to the weapon entity


    engine::Entity secondaryWeapon = registry.createEntity();
    engine::components::Weapon sWeapon;
    sWeapon.fireRate = 1 / 2.5f; 
    sWeapon.currentCooldown = sWeapon.fireRate;
    sWeapon.damage = 100.0f; 

    registry.addComponent(secondaryWeapon, sWeapon);
    addWeaponToEntity(entity, 1, secondaryWeapon);
    
}

void Game::removePlane(engine::Entity planeEntityID) {
	if (planeEntityID == playerPlane) {
		std::cerr << "Warning -- Removed current player plane! Did you mean to do that?\n";
	}

    const bool isPlane = registry.hasComponent<engine::components::PlayerPlaneController>(planeEntityID);
    registry.destroyEntity(planeEntityID);
    if (isPlane) {
        PlaneFactory::decrementPlaneAmount();
    }
}

bool Game::changePlayerPlane(engine::Entity planeEntityID) { 
	if (!registry.isAlive(planeEntityID)) {
		std::cout << "Plane with ID: " << std::to_string(planeEntityID.id) << " does not exist and cannot be set to be new player plane\n";
        std::cout << "Current player ID: " << std::to_string(playerPlane.id) << "\n";
		return false;
	}
	if (playerPlane == planeEntityID) {
		std::cout << "Tried to set player plane to current player plane\n";
		return false;
	}

	playerPlane = planeEntityID;
	return true; // The player's plane was changed
}

void Game::shootProjectile(engine::Entity shooterPlaneEntityID, ProjectileType projectileType, float muzzleUpRotation) {
    if (!registry.isAlive(shooterPlaneEntityID)) {
        std::cout << "Cannot shoot projectile from a non-existing plane with ID: " << std::to_string(shooterPlaneEntityID.id) << "\n";
        return;
    }
    engine::Entity entity = ProjectileFactory::spawnProjectile(registry, lifetimeSystem, now, shooterPlaneEntityID, projectileType);
}

// Update and draw
void Game::update(float dt) {
    ZoneScopedN("Update function");

    inputSystem.update(registry, dt, playerPrimaryAction, playerSecondaryAction);

    {
        ZoneScopedN("Combat");

        auto PPC = registry.getComponent<engine::components::PlayerPlaneController>(playerPlane);
        engine::components::AttackAction primaryAction = PPC.primaryAction;
        engine::components::AttackAction secondaryAction = PPC.secondaryAction;

        handleEntityAttackAction(playerPlane, primaryAction);
        handleEntityAttackAction(playerPlane, secondaryAction);
    }

    physicsSystem.update(registry, dt);
    particleTrailSystem.update(registry, lifetimeSystem, dt);
    lifetimeSystem.update(registry, dt);

    sf::Vector2f ppp = registry.getComponent<engine::components::Transform>(playerPlane).position;
    camera.update(ppp, dt);
    TerrainMaker::updateWorld(ppp.x, windowParams.width);

    terrainRenderer.updateVAO();

    now += dt; // increase global time counter
}

void Game::draw() {
    ZoneScopedN("Draw function");

    window.clear(sf::Color{255, 0, 0}); // For easy visibility of pixels that were not written to (makes it easier to see holes in the art)

    // World space
    window.setView(camera.getView());
    {
        ZoneScopedN("Background draw");
        background.draw(window, camera.getView());
    }
    {
        ZoneScopedN("Entity draw");
        // Render all entities
        renderSystem.render(registry, window, camera);
    }

    {
        ZoneScopedN("Terrain draw");
        terrainRenderer.draw(window);
    }

    // Screen space
    window.setView(window.getDefaultView());

    {
        ZoneScopedN("HUD draw");
        drawHud();
    }

    {
        ZoneScopedN("Display function");
        window.display();
    }
}

void Game::drawHud() {
    auto& ppTransform = registry.getComponent<engine::components::Transform>(playerPlane);
    auto& ppKinematics = registry.getComponent<engine::components::Kinematics>(playerPlane);

    auto getVector2fLength = [](const sf::Vector2f& vec) -> float {
        return std::sqrtf((vec.x * vec.x) + (vec.y * vec.y));
    };

    const sf::Vector2f position = ppTransform.position;
    const float speed = getVector2fLength(ppKinematics.velocity);

    std::ostringstream hud;
    hud << std::fixed 
        << std::setprecision(precision) << "FPS: " << FPS
        << std::setprecision(1) << "\nPlane x: " << position.x
        << "\nAltitude: " << -position.y
        << "\nAngle: " << -ppTransform.rotation.asDegrees()
        << "\nSpeed: " << speed
        << "\n"
        << "\nPlane amount: " << PlaneFactory::getPlaneAmount()
        << "\nProjectile amount: " << ProjectileFactory::getProjectileAmount()
        << " | culled total " << std::to_string(renderSystem.getCulledEntityAmount())
        << "\nParticle amount: " << ParticleFactory::getParticleAmount()
        ;

    text.setString(hud.str());
    window.draw(text);
}

void Game::run() {
    
    FrameLimiter frameLimiter(kFramerateLimit);
    // Disable SFML framerate limiters  
    window.setFramerateLimit(0);
    window.setVerticalSyncEnabled(false);

    // Initialize 
    InitializeAllEntityComponents();
    initializeFactoryTextures();

    engine::Entity PlaneID = PlaneFactory::spawnPlane(registry);
    changePlayerPlane(PlaneID);
    addWeaponsToEntity(playerPlane);
    camera.snapTo(registry.getComponent<engine::components::Transform>(playerPlane).position);

    TerrainMaker(0);

    FpsCounter fpsCounter;
    sf::Clock clock;
    setReportInterval(0.2f);
    setPrecision(2);

    auto lastTime = std::chrono::steady_clock::now();
    float dt;
    while (window.isOpen()) {
        {
            auto nowTime = std::chrono::steady_clock::now();
            dt = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime).count() / 1'000'000.0f; // in seconds
            lastTime = nowTime;
        }
        FrameMark;

        handleEvents();
        update(dt);
        draw();

        fpsCounter.tick();
        frameLimiter.wait();
    }
}

int main() {
    try {
        Game game;
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}