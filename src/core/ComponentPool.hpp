#ifndef ENGINE_CORE_COMPONENT_POOL_HPP
#define ENGINE_CORE_COMPONENT_POOL_HPP

#include <array>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Entity.hpp"
#include "IPool.hpp"

namespace engine {

// Sparse set: `sparse[id]` -> index into the dense `entities` / `components` arrays.
// The dense arrays are tightly packed, so iterating a pool is a linear memory walk.
template <typename T>
class ComponentPool final : public IPool {
public:
    ComponentPool() {
        sparse.fill(kAbsent);
    }

    void insert(Entity entity, T component) {
        validateEntity(entity);
        if (sparse[entity.id] != kAbsent) {
            throw std::logic_error("Entity already has this component");
        }

        sparse[entity.id] = components.size();
        entities.push_back(entity);
        components.push_back(std::move(component));
    }

    void remove(Entity entity) {
        validateEntity(entity);
        if (!contains(entity)) {
            throw std::out_of_range("Entity does not have this component");
        }

        const std::size_t index = sparse[entity.id];
        const std::size_t lastIndex = components.size() - 1;
        if (index != lastIndex) {
            components[index] = std::move(components[lastIndex]);
            entities[index] = entities[lastIndex];
            sparse[entities[index].id] = index;
        }

        components.pop_back();
        entities.pop_back();
        sparse[entity.id] = kAbsent;
    }

    T& get(Entity entity) {
        validateEntity(entity);
        if (!contains(entity)) {
            throw std::out_of_range("Entity does not have this component");
        }
        return components[sparse[entity.id]];
    }

    const T& get(Entity entity) const {
        validateEntity(entity);
        if (!contains(entity)) {
            throw std::out_of_range("Entity does not have this component");
        }
        return components[sparse[entity.id]];
    }

    // Only call after contains(entity) was true (queries and groups guarantee that).
    T& getUnchecked(Entity entity) noexcept { return components[sparse[entity.id]]; }
    const T& getUnchecked(Entity entity) const noexcept { return components[sparse[entity.id]]; }

    // Also compares the generation, so a stale handle to a reused slot is not a match.
    bool contains(Entity entity) const noexcept {
        if (entity.id >= MAX_ENTITIES) {
            return false;
        }
        const std::size_t index = sparse[entity.id];
        return index != kAbsent && entities[index] == entity;
    }

    void entityDestroyed(Entity entity) override {
        if (contains(entity)) {
            remove(entity);
        }
    }

    // --- Dense access -------------------------------------------------------------------
    // `for (auto& t : registry.components<Transform>())` walks the packed component array.
    // Don't add or remove components of this type while iterating.
    std::size_t size() const noexcept override { return components.size(); }
    const std::vector<Entity>& entityList() const noexcept override { return entities; }

    Entity entityAt(std::size_t index) const noexcept { return entities[index]; }
    T& dataAt(std::size_t index) noexcept { return components[index]; }
    const T& dataAt(std::size_t index) const noexcept { return components[index]; }

    typename std::vector<T>::iterator begin() noexcept { return components.begin(); }
    typename std::vector<T>::iterator end() noexcept { return components.end(); }
    typename std::vector<T>::const_iterator begin() const noexcept { return components.begin(); }
    typename std::vector<T>::const_iterator end() const noexcept { return components.end(); }

private:
    static constexpr std::size_t kAbsent = static_cast<std::size_t>(-1);

    static void validateEntity(Entity entity) {
        if (entity.id >= MAX_ENTITIES) {
            throw std::out_of_range("Entity ID is outside the supported range");
        }
    }

    std::array<std::size_t, MAX_ENTITIES> sparse{};
    std::vector<Entity> entities;
    std::vector<T> components;
};

} // namespace engine

#endif // ENGINE_CORE_COMPONENT_POOL_HPP