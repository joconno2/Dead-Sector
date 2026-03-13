#pragma once
#include "entities/HunterICE.hpp"
#include "entities/SentryICE.hpp"
#include "entities/SpawnerICE.hpp"
#include "entities/PhantomICE.hpp"
#include "entities/LeechICE.hpp"
#include "entities/MirrorICE.hpp"
#include "math/Vec2.hpp"
#include <memory>
#include <vector>
#include <random>

class SpawnManager {
public:
    struct Batch {
        std::vector<std::unique_ptr<HunterICE>>     hunters;
        std::vector<std::unique_ptr<SentryICE>>     sentries;
        std::vector<std::unique_ptr<SpawnerICE>>    spawnerICE;
        std::vector<std::unique_ptr<PhantomICE>>    phantoms;
        std::vector<std::unique_ptr<LeechICE>>      leeches;
        std::vector<std::unique_ptr<MirrorICE>>     mirrors;
    };

    SpawnManager();

    // Returns new entities to add this tick. currentCount = total live ICE.
    Batch update(float dt, float tracePct, int currentCount);

    void reset();

private:
    float        m_timer = 0.f;
    std::mt19937 m_rng;

    float intervalForTrace(float tracePct) const;
    void  spawnOne(float tracePct, Batch& out);
    Vec2  randomEdgePos();
    Vec2  velTowardCenter(Vec2 spawnPos, float speed);
};
