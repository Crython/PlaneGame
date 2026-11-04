#ifndef ENGINE_CORE_COMPONENT_TYPE_HPP
#define ENGINE_CORE_COMPONENT_TYPE_HPP

#include <cstddef>

namespace engine {

using ComponentType = std::size_t;
inline constexpr ComponentType MAX_COMPONENTS = 32;

inline ComponentType nextComponentType() noexcept {
    static ComponentType next = 0;
    return next++;
}

template <typename T>
ComponentType componentType() noexcept {
    static const ComponentType type = nextComponentType();
    return type;
}

} // namespace engine

#endif // ENGINE_CORE_COMPONENT_TYPE_HPP
