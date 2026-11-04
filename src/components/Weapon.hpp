#ifndef ENGINE_COMPONENTS_WEAPON_HPP
#define ENGINE_COMPONENTS_WEAPON_HPP



namespace engine::components {

struct Weapon {
    float fireRate = 0.0f;
    float currentCooldown = 0.0f;
    float damage = 0.0f;
};

} // namespace engine::components

#endif // ENGINE_COMPONENTS_WEAPON_HPP

