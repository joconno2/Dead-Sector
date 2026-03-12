#pragma once
#include "Entity.hpp"

class Projectile;

class Avatar : public Entity {
public:
    Avatar(float startX, float startY);

    void update(float dt) override;

    void applyThrust(float dt, float forceMult = 1.f, float speedCapMult = 1.f);
    void rotateLeft(float dt);
    void rotateRight(float dt);

    // Returns a heap-allocated Projectile with optional speed/radius multipliers.
    Projectile* fire(float speedMult = 1.f, float radiusMult = 1.f);

    bool  thrusting     = false;  // for thruster flame render
    float thrusterTimer = 0.f;    // flicker timer

    // Program effects
    bool  shielded       = false;
    float shieldTimer    = 0.f;   // remaining invincibility seconds
    float overdriveTimer = 0.f;   // remaining overdrive seconds

    // Mod effects
    int   extraLives     = 0;     // ADAPTIVE_ARMOR: absorbs 1 fatal hit per charge
};
