#include "SpawnManager.hpp"
#include "entities/PhantomICE.hpp"
#include "entities/LeechICE.hpp"
#include "entities/MirrorICE.hpp"
#include "core/Constants.hpp"
#include <cmath>
#include <algorithm>

SpawnManager::SpawnManager()
    : m_rng(std::random_device{}())
{}

void SpawnManager::reset() {
    m_timer = 0.f;
    m_rng.seed(std::random_device{}());
}

float SpawnManager::intervalForTrace(float tracePct) const {
    float t = std::clamp(tracePct / 100.f, 0.f, 1.f);
    return Constants::SPAWN_INTERVAL_BASE * (1.f - t)
         + Constants::SPAWN_INTERVAL_MIN  * t;
}

SpawnManager::Batch SpawnManager::update(float dt, float tracePct, int currentCount) {
    Batch out;

    if (currentCount >= Constants::SPAWN_MAX_ENTITIES) {
        m_timer += dt * 0.5f;
        return out;
    }

    m_timer += dt;
    float interval = intervalForTrace(tracePct);

    while (m_timer >= interval) {
        m_timer -= interval;
        int total = currentCount
                  + (int)out.hunters.size()
                  + (int)out.sentries.size()
                  + (int)out.spawnerICE.size()
                  + (int)out.phantoms.size()
                  + (int)out.leeches.size()
                  + (int)out.mirrors.size();
        if (total < Constants::SPAWN_MAX_ENTITIES)
            spawnOne(tracePct, out);
    }

    return out;
}

void SpawnManager::spawnOne(float tracePct, Batch& out) {
    Vec2 spawnPos = randomEdgePos();

    std::uniform_int_distribution<int> d(0, 99);
    int roll = d(m_rng);

    if (tracePct >= 75.f) {
        // 18% Hunter, 22% Sentry, 18% Spawner, 17% Phantom, 12% Leech, 13% Mirror
        if (roll < 18) {
            out.hunters.push_back(std::make_unique<HunterICE>(
                spawnPos, velTowardCenter(spawnPos, Constants::HUNTER_MAX_SPEED * 0.7f)));
        } else if (roll < 40) {
            out.sentries.push_back(std::make_unique<SentryICE>(
                spawnPos, velTowardCenter(spawnPos, 35.f)));
        } else if (roll < 58) {
            out.spawnerICE.push_back(std::make_unique<SpawnerICE>(
                spawnPos, velTowardCenter(spawnPos, 25.f)));
        } else if (roll < 75) {
            out.phantoms.push_back(std::make_unique<PhantomICE>(
                spawnPos, velTowardCenter(spawnPos, 40.f)));
        } else if (roll < 87) {
            out.leeches.push_back(std::make_unique<LeechICE>(
                spawnPos, velTowardCenter(spawnPos, Constants::HUNTER_MAX_SPEED * 0.5f)));
        } else {
            out.mirrors.push_back(std::make_unique<MirrorICE>(
                spawnPos, velTowardCenter(spawnPos, Constants::HUNTER_MAX_SPEED * 0.3f)));
        }
    } else if (tracePct >= 50.f) {
        // 50% Hunter, 50% Sentry
        if (roll < 50) {
            out.hunters.push_back(std::make_unique<HunterICE>(
                spawnPos, velTowardCenter(spawnPos, Constants::HUNTER_MAX_SPEED * 0.55f)));
        } else {
            out.sentries.push_back(std::make_unique<SentryICE>(
                spawnPos, velTowardCenter(spawnPos, 28.f)));
        }
    } else if (tracePct >= 25.f) {
        // 70% Hunter, 30% Sentry
        if (roll < 70) {
            out.hunters.push_back(std::make_unique<HunterICE>(
                spawnPos, velTowardCenter(spawnPos, Constants::HUNTER_MAX_SPEED * 0.4f)));
        } else {
            out.sentries.push_back(std::make_unique<SentryICE>(
                spawnPos, velTowardCenter(spawnPos, 22.f)));
        }
    } else {
        // Below 25%: Hunters only, slow approach
        out.hunters.push_back(std::make_unique<HunterICE>(
            spawnPos, velTowardCenter(spawnPos, Constants::HUNTER_MAX_SPEED * 0.25f)));
    }
}

Vec2 SpawnManager::randomEdgePos() {
    std::uniform_real_distribution<float> xd(0.f, Constants::SCREEN_WF);
    std::uniform_real_distribution<float> yd(0.f, Constants::SCREEN_HF);
    std::uniform_int_distribution<int>    ed(0, 3);
    constexpr float M = 35.f;
    switch (ed(m_rng)) {
        case 0: return { xd(m_rng), -M };
        case 1: return { xd(m_rng),  Constants::SCREEN_HF + M };
        case 2: return { -M,         yd(m_rng) };
        default:return { Constants::SCREEN_WF + M, yd(m_rng) };
    }
}

Vec2 SpawnManager::velTowardCenter(Vec2 spawnPos, float speed) {
    float cx = Constants::SCREEN_WF * 0.5f;
    float cy = Constants::SCREEN_HF * 0.5f;
    float dx = cx - spawnPos.x, dy = cy - spawnPos.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) len = 1.f;
    std::uniform_real_distribution<float> spread(-0.5f, 0.5f);
    float angle = std::atan2(dy, dx) + spread(m_rng);
    return { std::cos(angle) * speed, std::sin(angle) * speed };
}
