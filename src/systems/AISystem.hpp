#pragma once
#include "entities/HunterICE.hpp"
#include "entities/SentryICE.hpp"
#include "entities/SpawnerICE.hpp"
#include "entities/EnemyProjectile.hpp"
#include "math/Vec2.hpp"
#include <vector>
#include <memory>

class Avatar;

struct AIConfig {
    Vec2  decoyPos           = {-1.f, -1.f};
    bool  decoyActive        = false;
    float sentryFireRateMult = 1.f;   // >1 = slower (SIGNAL_JAM: 1.667)
};

class AISystem {
public:
    struct Result {
        std::vector<std::unique_ptr<EnemyProjectile>> firedProjectiles;
        std::vector<std::unique_ptr<HunterICE>>       spawnedHunters;
    };

    Result update(float dt,
                  std::vector<std::unique_ptr<HunterICE>>&  hunters,
                  std::vector<std::unique_ptr<SentryICE>>&  sentries,
                  std::vector<std::unique_ptr<SpawnerICE>>& spawners,
                  const Avatar* avatar,
                  AIConfig cfg = AIConfig{});

private:
    void updateHunter (float dt, HunterICE&  h, Vec2 targetPos);
    void updateSentry (float dt, SentryICE&  s, const Avatar& av, Result& out,
                       float fireRateMult);
    void updateSpawner(float dt, SpawnerICE& sp, Result& out);
};
