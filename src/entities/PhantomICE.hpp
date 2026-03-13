#pragma once
#include "Entity.hpp"

// Cloaked hunter — invisible until within 120 px of the avatar.
// Blinks (flickers visible) for 0.5 s before firing an enemy projectile,
// then re-cloaks.
class PhantomICE : public Entity {
public:
    explicit PhantomICE(Vec2 pos, Vec2 initialVel = {});
    void update(float dt) override;
    int  scoreValue() const override { return 175; }

    // Set by AISystem each frame based on distance to avatar
    bool  nearPlayer  = false;     // true when within REVEAL_RADIUS

    // Set by AISystem when it wants to fire
    bool  wantsToFire = false;

    // Visibility — 0=fully cloaked, 1=fully visible
    // CombatScene render reads this to set draw alpha.
    float visibility  = 0.f;

    float fireCooldown = 2.5f;    // managed by AISystem

    // Blink phase: >0 means actively blinking (counting down to fire)
    float blinkTimer  = 0.f;

    static constexpr float REVEAL_RADIUS = 120.f;
    static constexpr float BLINK_DURATION = 0.5f;
};
