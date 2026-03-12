#pragma once
#include "Entity.hpp"

// Actively chases the player. AISystem applies steering each frame.
class HunterICE : public Entity {
public:
    explicit HunterICE(Vec2 pos, Vec2 initialVel = {});
    void update(float dt) override;
    int  scoreValue() const override { return 75; }
};
