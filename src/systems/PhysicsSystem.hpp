#ifndef ENGINE_SYSTEMS_PHYSICS_SYSTEM_HPP
#define ENGINE_SYSTEMS_PHYSICS_SYSTEM_HPP

namespace engine {
class Registry;
}

namespace engine::systems {

class PhysicsSystem {
public:
    void update(Registry& registry, float deltaTime) const;
};

} // namespace engine::systems

#endif // ENGINE_SYSTEMS_PHYSICS_SYSTEM_HPP
