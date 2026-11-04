#ifndef ENGINE_CORE_ENTITY_HPP
#define ENGINE_CORE_ENTITY_HPP

#include <cstdint>

namespace engine {

inline constexpr std::uint32_t MAX_ENTITIES = 1'000'000; 
inline constexpr std::uint32_t NULL_ENTITY_ID = 0xFFFFFFFFu;

// A handle = slot (id) + generation. When a slot is reused its generation goes up,
// so old handles to the previous occupant stop matching and are detected as dead.
struct Entity {
    std::uint32_t id = NULL_ENTITY_ID;   // default-constructed Entity is the "null" handle
    std::uint32_t generation = 0;

    bool operator==(Entity other) const { return id == other.id && generation == other.generation; }
    bool operator!=(Entity other) const { return !(*this == other); }
    bool isNull() const { return id == NULL_ENTITY_ID; }
};

inline constexpr Entity NULL_ENTITY{};

} // namespace engine

#endif // ENGINE_CORE_ENTITY_HPP