#pragma once
#include "Entity.hpp"

// Drifts and periodically emits DataConstruct fragments. Kill it first.
class SpawnerICE : public Entity {
public:
    explicit SpawnerICE(Vec2 pos, Vec2 vel = {});
    void update(float dt) override;
    int  scoreValue() const override { return 200; }

    float spawnCooldown  = 1.0f;   // first spawn delay (shorter to make presence felt)
    bool  wantsToSpawn   = false;  // set by AISystem
    int   spawnedCount   = 0;
    static constexpr int MAX_CHILDREN = 4;
};
