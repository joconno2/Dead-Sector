#pragma once
#include "Entity.hpp"

// Slowly drifts toward the player, face always rotating to track them.
// Has destructible armor — takes 3 hits to destroy.
class MirrorICE : public Entity {
public:
    explicit MirrorICE(Vec2 pos, Vec2 initialVel = {});
    void update(float dt) override;
    int  scoreValue() const override { return 25; }

    float facingAngle  = 0.f;   // angle the mirror face points (toward player each frame)
    int   hp           = 3;     // hits required to destroy
    float hitFlashTimer = 0.f;  // >0 while showing damage flash
};
