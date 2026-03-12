#pragma once
#include "Entity.hpp"

// Stationary or slow-drifting turret. AISystem rotates it to aim and fires.
class SentryICE : public Entity {
public:
    explicit SentryICE(Vec2 pos, Vec2 driftVel = {});
    void update(float dt) override;
    int  scoreValue() const override { return 150; }

    float fireCooldown = 1.5f;   // first shot delay
    bool  wantsToFire  = false;  // set true by AISystem when cooldown expires
};
