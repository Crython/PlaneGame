#ifndef ENGINE_SYSTEMS_LIFETIME_SYSTEM_HPP
#define ENGINE_SYSTEMS_LIFETIME_SYSTEM_HPP

#include <vector>
#include <chrono>
#include <queue>

#include "components/Lifetime.hpp"
#include "core/Registry.hpp"

namespace engine {
class Registry;
}

namespace engine::systems {

class LifetimeSystem {
public:
    LifetimeSystem() {
        expiredEntities.reserve(engine::MAX_ENTITIES / 10); // Reserve a tiny bit of space for expiered entities
    }

    // Deletes expired entities (can also be called the cleanup system)
    void update(Registry& registry, float deltaTime) const;
    void schedule(Entity e, float lifetime, float now) { expiryQueue.push({now + lifetime, e}); }

private:
    mutable float now = 0;


    struct Expiry {
        float time;
        Entity entity;

        bool operator>(const Expiry& o) const { return time > o.time; }
    };

    mutable std::vector<Entity> expiredEntities;
    mutable std::priority_queue<Expiry, std::vector<Expiry>, std::greater<>> expiryQueue;

};

} // namespace engine::systems

#endif // ENGINE_SYSTEMS_LIFETIME_SYSTEM_HPP
