#ifndef ENGINE_SYSTEMS_PLANE_FLIGHT_HPP
#define ENGINE_SYSTEMS_PLANE_FLIGHT_HPP

#include <algorithm>
#include <cmath>

// Side-view flight model for the player's plane, written with plain floats so it can be tested
// without SFML. Coordinates follow SFML: +Y points down, heading 0 = facing right, positive = clockwise.
//
// The idea: a plane is not a spaceship. Velocity is pulled round to where the nose points,
// lift (perpendicular to the wings) fights gravity once there is enough airspeed, drag grows with
// speed squared, and a stalled plane drops its nose.
namespace engine::systems::flight {

struct Vec {
    float x = 0.0f;
    float y = 0.0f;
};

struct Params {
    float thrust = 135.0f;          // acceleration at full throttle (units/s^2)
    float maxSpeed = 360.0f;        // level-flight speed at which drag cancels full thrust
    float gravity = 60.0f;          // units/s^2
    float levelSpeed = 140.0f;      // forward speed at which lift exactly balances gravity (faster doesn't add lift)
    float stallSpeed = 90.0f;       // below this, lift fades out and the nose starts to drop
    float sideDamping = 3.0f;       // 1/s: how quickly velocity swings round to the nose (higher = more "on rails")
    float turnEfficiency = 0.85f;   // 0..1: share of the sideways energy that ends up as forward speed
    float noseDropRate = 2.5f;      // 1/s: how hard a fully stalled nose falls toward the flight path
};

inline float smoothstep(float edge0, float edge1, float x) {
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Advances velocity and heading by dt. Position integration is left to the caller.
inline void step(Vec& velocity, float& heading, float throttle, float dt, const Params& p) {
    constexpr float kPi = 3.14159265358979f;

    const float fx = std::cos(heading), fy = std::sin(heading);   // along the nose
    const float nx = fy, ny = -fx;                                // "up" as seen from the cockpit

    float vf = velocity.x * fx + velocity.y * fy;   // speed along the nose (airspeed)
    float vn = velocity.x * nx + velocity.y * ny;   // sideways speed (slip)

    // Thrust and drag act along the nose; drag ~ speed^2 gives a natural top speed.
    vf += throttle * p.thrust * dt;
    const float dragK = p.thrust / (p.maxSpeed * p.maxSpeed);
    vf -= dragK * vf * std::fabs(vf) * dt;

    // Gravity, split into the nose and sideways directions.
    vf += p.gravity * fy * dt;
    vn += p.gravity * ny * dt;

    // Lift: perpendicular to the wings, balances gravity at levelSpeed, fades with airspeed.
    const float airspeed = std::max(vf, 0.0f);
    const float lift = p.gravity * std::min(1.0f, (airspeed / p.levelSpeed) * (airspeed / p.levelSpeed));
    vn += lift * dt;

    // Swing the velocity round to the nose: sideways speed decays, most of it becomes forward speed.
    const float newVn = vn * std::exp(-p.sideDamping * dt);
    const float moved = vn * vn - newVn * newVn;
    vf = std::copysign(std::sqrt(vf * vf + p.turnEfficiency * moved), vf);
    vn = newVn;

    velocity.x = fx * vf + nx * vn;
    velocity.y = fy * vf + ny * vn;

    // Stall: the nose falls toward wherever the plane is actually going.
    const float stall = 1.0f - smoothstep(0.6f * p.stallSpeed, p.stallSpeed, vf);
    if (stall > 0.0f && std::hypot(velocity.x, velocity.y) > 1.0f) {
        const float flightAngle = std::atan2(velocity.y, velocity.x);
        const float diff = std::remainder(flightAngle - heading, 2.0f * kPi);
        heading += diff * (1.0f - std::exp(-p.noseDropRate * stall * dt));
    }
}

} // namespace engine::systems::flight

#endif // ENGINE_SYSTEMS_PLANE_FLIGHT_HPP