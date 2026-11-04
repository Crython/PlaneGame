#ifndef ENGINE_CORE_REGISTRY_HPP
#define ENGINE_CORE_REGISTRY_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>
#include <unordered_map>

#include "ComponentPool.hpp"
#include "ComponentType.hpp"
#include "Entity.hpp"
#include "Group.hpp"
#include "IPool.hpp"

#include "tracy-0.14.1/public/tracy/Tracy.hpp" // Used for accurate and easy CPU timings

namespace engine {

static_assert(MAX_COMPONENTS <= 32, "Component signatures are stored in 32-bit masks");

class Registry {
public:
    Registry();

    Entity createEntity();
    void destroyEntity(Entity entity);
    bool isAlive(Entity entity) const noexcept;

    const std::vector<Entity>& entities() const noexcept {
        return activeEntities;
    }

    template <typename T>
    void registerComponent() {
        const ComponentType type = componentType<T>();
        if (type >= MAX_COMPONENTS) {
            throw std::length_error("Maximum number of component types exceeded");
        }
        if (!componentPools[type]) {
            componentPools[type] = std::make_unique<ComponentPool<T>>();
        }
    }

    template <typename T>
    void addComponent(Entity entity, T component) {
        validateAlive(entity);
        pool<T>().insert(entity, std::move(component));
        signatures[entity.id] |= bitOf<T>();
        refreshGroups(entity);
    }

    template <typename T>
    void removeComponent(Entity entity) {
        validateAlive(entity);
        pool<T>().remove(entity);
        signatures[entity.id] &= ~bitOf<T>();
        refreshGroups(entity);
    }

    template <typename T>
    T& getComponent(Entity entity) {
        validateAlive(entity);
        return pool<T>().get(entity);
    }

    template <typename T>
    const T& getComponent(Entity entity) const {
        validateAlive(entity);
        return pool<T>().get(entity);
    }

    template <typename T>
    bool hasComponent(Entity entity) const {
        auto c = pool<T>().contains(entity);
        if (!c) {
            return false;
        }
        return c;
    }

    // --- Queries ---------------------------------------------------------------------------
    //
    // None of these may be used to create/destroy entities or add/remove the iterated component
    // types from inside the callback. Collect them and do it afterwards (as LifetimeSystem does).

    // Direct access to the packed array of one component type: `for (auto& t : registry.components<Transform>())`
    template <typename T>
    ComponentPool<T>& components() {
        return pool<T>();
    }

    // Ad-hoc query: fn(Entity, Ts&...).
    //  - One type: walks that pool's dense array directly.
    //  - Several types: walks the smallest of the pools and skips entities missing any other type.
    template <typename... Ts, typename Fn>
    void each(Fn&& fn) {
        static_assert(sizeof...(Ts) > 0, "each<>() needs at least one component type");

        if constexpr (sizeof...(Ts) == 1) {
            using First = std::tuple_element_t<0, std::tuple<Ts...>>;
            auto& p = pool<First>();
            for (std::size_t i = 0; i < p.size(); ++i) {
                fn(p.entityAt(i), p.dataAt(i));
            }
        } else {
            auto pools = std::tuple<ComponentPool<Ts>&...>{pool<Ts>()...};
            const IPool& driver = smallestPool<Ts...>();
            for (const Entity entity : driver.entityList()) {
                std::apply([&](auto&... p) {
                    if ((p.contains(entity) && ...)) {
                        fn(entity, p.getUnchecked(entity)...);
                    }
                }, pools);
            }
        }
    }

    // Cached query for the hot component combinations. The first call builds the group from the
    // existing entities; after that the Registry keeps it up to date. Cheap to call every frame.
    template <typename... Ts>
    GroupView<Ts...> group() {
        static_assert(sizeof...(Ts) > 0, "group<>() needs at least one component type");

        const std::uint32_t mask = (bitOf<Ts>() | ...);

        Group* found = nullptr;
        for (const auto& g : groups) {
            if (g->mask() == mask && g->excludedMask() == 0) {
                found = g.get();
                break;
            }
        }
        if (!found) {
            groups.push_back(std::make_unique<Group>(mask));
            found = groups.back().get();
            for (const Entity entity : activeEntities) {
                found->refresh(entity, signatures[entity.id], isAlive(entity));
            }
        }
        return GroupView<Ts...>(*found, pool<Ts>()...);
    }

    // Cached query for entities that have all Ts and do not have Excluded.
    // With no Ts, this iterates every live entity lacking Excluded.
    template <typename Excluded, typename... Ts>
    GroupView<Ts...> groupWithout() {
        const std::uint32_t mask = (std::uint32_t{0} | ... | bitOf<Ts>());
        const std::uint32_t excludedMask = bitOf<Excluded>();

        Group* found = nullptr;
        for (const auto& g : groups) {
            if (g->mask() == mask && g->excludedMask() == excludedMask) {
                found = g.get();
                break;
            }
        }
        if (!found) {
            groups.push_back(std::make_unique<Group>(mask, excludedMask));
            found = groups.back().get();
            for (const Entity entity : activeEntities) {
                found->refresh(entity, signatures[entity.id], isAlive(entity));
            }
        }
        return GroupView<Ts...>(*found, pool<Ts>()...);
    }

private:
    struct EntityHash {
        std::size_t operator()(Entity entity) const noexcept {
            return (std::hash<std::uint32_t>{}(entity.id) << 1) ^
                   std::hash<std::uint32_t>{}(entity.generation);
        }
    };

    template <typename T>
    ComponentPool<T>& pool() {
        const ComponentType type = componentType<T>();
        
        return *static_cast<ComponentPool<T>*>(componentPools[type].get());
    }

    template <typename T>
    const ComponentPool<T>& pool() const {
        const ComponentType type = componentType<T>();
        if (type >= MAX_COMPONENTS || !componentPools[type]) {
            throw std::logic_error("Component type has not been registered");
        }
        return *static_cast<const ComponentPool<T>*>(componentPools[type].get());
    }

    // Bit used for T in entity signatures and group masks. Throws if T was never registered.
    template <typename T>
    std::uint32_t bitOf() const {
        const ComponentType type = componentType<T>();
        if (type >= MAX_COMPONENTS || !componentPools[type]) {
            throw std::logic_error("Component type has not been registered");
        }
        return std::uint32_t{1} << type;
    }

    template <typename... Ts>
    const IPool& smallestPool() const {
        const IPool* pools[] = {&pool<Ts>()...};
        const IPool* best = pools[0];
        for (const IPool* p : pools) {
            if (p->size() < best->size()) {
                best = p;
            }
        }
        return *best;
    }

    bool add(Entity entity) {
        // If the element already exists, don't add it
        if (entityToIndex.find(entity) != entityToIndex.end()) {
            return false; 
        }
        
        // Add to the end of the vector and record its index in the map
        activeEntities.push_back(entity);
        entityToIndex[entity] = activeEntities.size() - 1;
        return true;
    }

    // Removes an element from the collection. Returns true if successful.
    bool remove(Entity entity) {
        // If the element does not exist, we cannot remove it
        auto it = entityToIndex.find(entity);
        if (it == entityToIndex.end()) {
            return false;
        }
        
        // Get the index of the element to delete and the value of the last element
        const std::size_t indexToDelete = it->second;
        Entity lastElement = activeEntities.back();
        
        // Move the last element to the slot of the element we want to delete
        activeEntities[indexToDelete] = lastElement;
        entityToIndex[lastElement] = indexToDelete; // Update map for the moved element
        
        // Remove the last element from both structures
        activeEntities.pop_back();
        entityToIndex.erase(it); // Erase by iterator is O(1)
        
        return true;
    }

    void refreshGroups(Entity entity);
    void validateAlive(Entity entity) const;
    bool softValidateAlive(Entity entity) const noexcept;

    std::array<bool, MAX_ENTITIES> alive{};
    std::vector<std::uint32_t> generations;   // current generation of every slot
    std::vector<std::uint32_t> signatures;    // bit i set = entity has component type i (for more components, switch to uint64_t)
    std::array<std::unique_ptr<IPool>, MAX_COMPONENTS> componentPools{};
    std::vector<std::unique_ptr<Group>> groups;
    std::vector<std::uint32_t> availableIds;
    std::vector<Entity> activeEntities;
    std::unordered_map<Entity, std::size_t, EntityHash> entityToIndex;
};

} // namespace engine

#endif // ENGINE_CORE_REGISTRY_HPP