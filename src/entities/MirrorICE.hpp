#pragma once
#include "Entity.hpp"

// Slowly drifts toward the player, face always rotating to track them.
// Projectiles hitting the front face are reflected back; hits from behind deal damage.
class MirrorICE : public Entity {
public:
    explicit MirrorICE(Vec2 pos, Vec2 initialVel = {});
    void update(float dt) override;
    int  scoreValue() const override { return 10; }

    float facingAngle = 0.f;   // angle the mirror face points (toward player each frame)
};
