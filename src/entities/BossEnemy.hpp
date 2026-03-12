#pragma once
#include "Entity.hpp"
#include "EnemyProjectile.hpp"
#include "math/Vec2.hpp"
#include <array>
#include <memory>
#include <vector>

enum class BossType { Manticore, Archon, Vortex };

class BossEnemy : public Entity {
public:
    explicit BossEnemy(BossType type, Vec2 spawnPos);

    BossType bossType;
    int      hp    = 0;
    int      maxHp = 0;

    const char* bossName() const;

    struct UpdateResult {
        std::vector<std::unique_ptr<EnemyProjectile>> fired;
    };
    void update(float /*dt*/) override {}
    UpdateResult update(float dt, Vec2 playerPos);
    void render(struct VectorRenderer* vr, float time) const;

    // ARCHON: try to hit an orbiting shield first. Returns true if shield was hit.
    bool tryHitShield(Vec2 projPos, float projRadius);

    // True when the core can be damaged (ARCHON: all shields down; others: always)
    bool coreExposed() const;

    int scoreValue() const override { return 2000; }

private:
    float m_orbitAngle  = 0.f;   // boss orbit around screen centre
    float m_spinAngle   = 0.f;   // body/blade spin
    float m_fireTimer   = 0.f;

    // ARCHON: 4 orbiting shields
    std::array<bool, 4>  m_shields    = {true, true, true, true};

    // VORTEX: 4 heads (cardinal), each with their own fire timer
    std::array<bool,  4> m_heads      = {true, true, true, true};
    std::array<float, 4> m_headFire   = {0.f, 0.9f, 1.8f, 2.7f};

    Vec2 shieldPos(int i) const;
    Vec2 headPos  (int i) const;

    void updateManticore(float dt, Vec2 playerPos, UpdateResult& out);
    void updateArchon   (float dt, Vec2 playerPos, UpdateResult& out);
    void updateVortex   (float dt, Vec2 playerPos, UpdateResult& out);

    void renderManticore(struct VectorRenderer* vr, float time) const;
    void renderArchon   (struct VectorRenderer* vr, float time) const;
    void renderVortex   (struct VectorRenderer* vr, float time) const;
};
