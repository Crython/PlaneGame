#include "../PhysicsSystem.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "../PlaneFlight.hpp"
#include "../../components/Transform.hpp"
#include "../../components/Kinematics.hpp"
#include "../../components/PlayerController.hpp"
#include "../../core/Registry.hpp"

#include "tracy-0.14.1/public/tracy/Tracy.hpp" // Used for accurate and easy CPU timings

namespace engine::systems {

namespace {

constexpr float kProjectileGravity = 23.7f;
constexpr float kProjectileDrag = 0.001f;   // 1/s
constexpr float kMaxStep = 1.0f / 30.0f;   // a hitch (window drag, breakpoint) must not become one giant step
constexpr flight::Params kPlaneFlight{};   // tune the feel in PlaneFlight.hpp

} // namespace

void PhysicsSystem::update(Registry& registry, float deltaTime) const {
    if (deltaTime < 0.0f) {
        throw std::invalid_argument("Physics delta time cannot be negative");
    }
    deltaTime = std::min(deltaTime, kMaxStep);

    /* performance (25% of scope)
    ToDo: Iterate over all entities with the transform kinematics, but NO playerController
          Then iterate over the playerControllers (usually 1)

        - Add arg to registry.group to exclude entities with select components
            * Implementation cannot have expensive checks (like hasComponent)
    */
    auto updateMovement = [&deltaTime](components::Transform& transform, components::Kinematics& kinematics) {
        auto fastExp = [](float x) -> float {
            // Normal case
            if (x <= 0.5f) {
                return 1.0f - x * (1.0f - 0.5f * x);
                // return 1.0f - dt * (1.0f - dt * (0.5f - 0.16666667f * dt)); // 3rd degree approximation, a bit slower, a lot more accurate
            }

            // Rare case during massive lag spikes (FPS <= 2)
            return std::exp(x); // around 25x slower than the approximation above
        };

        float baseDrag = fastExp(-kProjectileDrag * deltaTime);

        // Integrate constant jerk over the step so it contributes to velocity immediately.
        kinematics.velocity += kinematics.acceleration * deltaTime
            + kinematics.jerk * (0.5f * deltaTime * deltaTime);
        kinematics.acceleration += kinematics.jerk * deltaTime;

        // Drag
        kinematics.velocity *= baseDrag;
        
        kinematics.velocity.y += kProjectileGravity * transform.gravityScale * deltaTime; // Gravity
        transform.position += kinematics.velocity * deltaTime; // Apply movement

        // Simple simulated ground
        if (transform.position.y >= -10) transform.position.y = -10;
    };

    {
        ZoneScopedN("Entity update loop");

        // Hot pair: cached group of every entity that has both Transform and Kinematics.
        registry.groupWithout<components::PlayerPlaneController, components::Transform, components::Kinematics>().each(
            [&](Entity entity, components::Transform& transform, components::Kinematics& kinematics) {
        
            updateMovement(transform, kinematics);
        });
    }

    
    registry.group<components::PlayerPlaneController, components::Transform, components::Kinematics>().each(
        [&](Entity entity, components::PlayerPlaneController& controller, components::Transform& transform, components::Kinematics& kinematics) {
            ZoneScopedN("Player Update");

            // Player input turns the nose first; the flight model then decides where the plane goes.
            constexpr float kTwoPi = 6.28318530718f;
            float heading = transform.rotation.asRadians() + controller.rotation.asRadians();
            heading = std::remainder(heading, kTwoPi);   // keep the angle bounded

            flight::Vec v{kinematics.velocity.x, kinematics.velocity.y};
            flight::step(v, heading, controller.throttle, deltaTime, kPlaneFlight);

            kinematics.velocity = {v.x, v.y};
            transform.rotation = sf::radians(heading);

            updateMovement(transform, kinematics);
        });
    }

} // namespace engine::systems