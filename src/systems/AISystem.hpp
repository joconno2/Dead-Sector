#pragma once
#include "entities/HunterICE.hpp"
#include "entities/SentryICE.hpp"
#include "entities/SpawnerICE.hpp"
#include "entities/EnemyProjectile.hpp"
#include <vector>
#include <memory>

class Avatar;

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
                  const Avatar* avatar);

private:
    void updateHunter (float dt, HunterICE&  h, const Avatar& av);
    void updateSentry (float dt, SentryICE&  s, const Avatar& av, Result& out);
    void updateSpawner(float dt, SpawnerICE& sp,                   Result& out);
};
