#pragma once
#include "Entity.hpp"
#include "core/Constants.hpp"

class Projectile : public Entity {
public:
    Projectile(Vec2 startPos, Vec2 startVel, float angle);
    void update(float dt) override;

    float lifetime = Constants::PROJ_LIFETIME;
};
