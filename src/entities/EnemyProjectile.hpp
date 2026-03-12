#pragma once
#include "Entity.hpp"

class EnemyProjectile : public Entity {
public:
    EnemyProjectile(Vec2 pos, Vec2 vel);
    void update(float dt) override;

    float lifetime;
};
