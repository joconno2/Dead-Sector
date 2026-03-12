#pragma once
#include "math/Vec2.hpp"
#include <vector>

enum class EntityType {
    Avatar, Projectile, DataConstruct,
    EnemyProjectile,
    HunterICE, SentryICE, SpawnerICE
};

class Entity {
public:
    virtual ~Entity() = default;
    virtual void update(float dt) = 0;
    virtual int  scoreValue() const { return 0; }

    const std::vector<Vec2>& worldVerts() const { return m_worldVerts; }
    void refreshVerts() { transformVerts(); }

    Vec2       pos;
    Vec2       vel;
    float      angle  = 0.f;
    float      radius = 0.f;
    bool       alive  = true;
    EntityType type;

protected:
    std::vector<Vec2> m_localVerts;
    std::vector<Vec2> m_worldVerts;

    void transformVerts();
};
