#ifndef ENGINE_CORE_GROUP_HPP
#define ENGINE_CORE_GROUP_HPP

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

#include "ComponentPool.hpp"
#include "Entity.hpp"

namespace engine {

// A cached list of every live entity that has ALL components in `mask` and NONE in `excludedMask`.
// The Registry keeps it up to date whenever a component is added/removed or an entity dies,
// so iterating it needs no per-entity filtering. Membership changes are O(1) (swap-remove),
// which means the order of members is not stable.
class Group {
public:
    explicit Group(std::uint32_t requiredMask, std::uint32_t excludedMask = 0)
        : mask_(requiredMask), excludedMask_(excludedMask), slot_(MAX_ENTITIES, kAbsent) {}

    std::uint32_t mask() const noexcept { return mask_; }
    std::uint32_t excludedMask() const noexcept { return excludedMask_; }
    const std::vector<Entity>& members() const noexcept { return members_; }

    // Re-checks one entity against the group after its component signature changed.
    void refresh(Entity entity, std::uint32_t signature, bool alive) {
        const bool matches = alive && (signature & mask_) == mask_ &&
                             (signature & excludedMask_) == 0;
        const bool present = slot_[entity.id] != kAbsent;

        if (matches && !present) {
            slot_[entity.id] = static_cast<std::uint32_t>(members_.size());
            members_.push_back(entity);
        } else if (!matches && present) {
            const std::uint32_t index = slot_[entity.id];
            const Entity last = members_.back();
            members_[index] = last;
            slot_[last.id] = index;
            members_.pop_back();
            slot_[entity.id] = kAbsent;
        }
    }

private:
    static constexpr std::uint32_t kAbsent = 0xFFFFFFFFu;

    std::uint32_t mask_;
    std::uint32_t excludedMask_;
    std::vector<Entity> members_;
    std::vector<std::uint32_t> slot_;   // entity id -> index in members_
};

// Typed, cheap-to-copy handle onto a Group plus the pools it needs.
// Don't create/destroy entities or add/remove grouped components inside each().
template <typename... Ts>
class GroupView {
public:
    GroupView(const Group& group, ComponentPool<Ts>&... pools)
        : group_(group), pools_(pools...) {}

    std::size_t size() const noexcept { return group_.members().size(); }

    // fn(Entity, Ts&...)
    template <typename Fn>
    void each(Fn&& fn) const {
        for (const Entity entity : group_.members()) {
            std::apply([&](auto&... pool) { fn(entity, pool.getUnchecked(entity)...); }, pools_);
        }
    }

private:
    const Group& group_;
    std::tuple<ComponentPool<Ts>&...> pools_;
};

} // namespace engine

#endif // ENGINE_CORE_GROUP_HPP