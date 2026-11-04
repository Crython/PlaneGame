#ifndef ENGINE_CORE_I_POOL_HPP
#define ENGINE_CORE_I_POOL_HPP

#include <cstddef>
#include <vector>

#include "Entity.hpp"

namespace engine {

class IPool {
public:
    virtual ~IPool() = default;
    virtual void entityDestroyed(Entity entity) = 0;

    // Number of stored components, and the owners in the same (dense) order.
    // Used to pick the smallest pool when iterating several component types.
    virtual std::size_t size() const noexcept = 0;
    virtual const std::vector<Entity>& entityList() const noexcept = 0;
};

} // namespace engine

#endif // ENGINE_CORE_I_POOL_HPP