#include "Registry.hpp"

#include <stdexcept>
#include <iostream>

namespace engine {

Registry::Registry()
    : generations(MAX_ENTITIES, 0u),
      signatures(MAX_ENTITIES, 0u) {
    availableIds.reserve(MAX_ENTITIES);
    for (std::uint32_t id = MAX_ENTITIES; id > 0; --id) {
        availableIds.push_back(id - 1);
    }
}

Entity Registry::createEntity() {
    ZoneScopedN("Registry create entity");
    if (availableIds.empty()) {
        throw std::length_error("Maximum number of entities exceeded");
    }

    const std::uint32_t id = availableIds.back();
    availableIds.pop_back();

    const Entity entity{id, generations[id]};
    if (!add(entity)) {
        availableIds.push_back(id);
        throw std::logic_error("Entity is already registered");
    }
    alive[id] = true;
    signatures[id] = 0;
    refreshGroups(entity);
    return entity;
}

void Registry::destroyEntity(Entity entity) {
    ZoneScopedN("Registry destroy entity");
    validateAlive(entity);
    if (!remove(entity)) {
        throw std::logic_error("Entity is not registered");
    }

    for (const auto& pool : componentPools) {
        if (pool) {
            pool->entityDestroyed(entity);
        }
    }

    // An empty signature makes every group drop the entity.
    signatures[entity.id] = 0;
    alive[entity.id] = false;
    ++generations[entity.id];   // every existing handle to this slot is now stale
    refreshGroups(entity);
    availableIds.push_back(entity.id);
}

bool Registry::isAlive(Entity entity) const noexcept {
    return entity.id < MAX_ENTITIES && alive[entity.id] && generations[entity.id] == entity.generation;
}

void Registry::refreshGroups(Entity entity) {
    for (const auto& group : groups) {
        group->refresh(entity, signatures[entity.id], isAlive(entity));
    }
}

void Registry::validateAlive(Entity entity) const {
    if (!isAlive(entity)) {
        throw std::out_of_range("Entity is not alive");
    }
}

bool Registry::softValidateAlive(Entity entity) const noexcept {
    if (!isAlive(entity)) {
        std::cout << "Entity is not alive, skipping entity\n";
        return false;
    }

    return true;
}

} // namespace engine