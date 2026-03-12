#pragma once
#include "IScene.hpp"
#include "world/Node.hpp"
#include "core/Constants.hpp"
#include "systems/PhysicsSystem.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/TraceSystem.hpp"
#include "systems/SpawnManager.hpp"
#include "systems/AISystem.hpp"
#include "systems/ProgramSystem.hpp"
#include "systems/ParticleSystem.hpp"
#include "entities/Avatar.hpp"
#include "entities/Projectile.hpp"
#include "entities/HunterICE.hpp"
#include "entities/SentryICE.hpp"
#include "entities/SpawnerICE.hpp"
#include "entities/EnemyProjectile.hpp"
#include <memory>
#include <vector>

// Configuration passed from MapScene when jacking into a node
struct NodeConfig {
    int           nodeId         = 0;
    int           tier           = 1;
    NodeObjective objective      = NodeObjective::Sweep;
    int           sweepTarget    = 6;
    float         surviveSeconds = 30.f;
    float         traceTickRate  = 3.0f;   // %/sec

};

class CombatScene : public IScene {
public:
    explicit CombatScene(NodeConfig cfg = {});

    void onEnter(SceneContext& ctx) override;
    void onExit()                   override;
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    NodeConfig      m_config;

    PhysicsSystem   m_physics{Constants::SCREEN_WF, Constants::SCREEN_HF};
    CollisionSystem m_collision;
    TraceSystem     m_trace;
    SpawnManager    m_spawner;
    AISystem        m_ai;

    ParticleSystem  m_particles;

    std::unique_ptr<Avatar>                       m_avatar;
    std::vector<std::unique_ptr<Projectile>>      m_projectiles;
    std::vector<std::unique_ptr<HunterICE>>       m_hunters;
    std::vector<std::unique_ptr<SentryICE>>       m_sentries;
    std::vector<std::unique_ptr<SpawnerICE>>      m_spawnerICE;
    std::vector<std::unique_ptr<EnemyProjectile>> m_enemyProjectiles;

    // Staging buffers — flushed after update to avoid iterator invalidation
    std::vector<std::unique_ptr<HunterICE>>       m_pendingHunters;

    int   m_score        = 0;
    int   m_iceKilled    = 0;   // for Sweep/Boss objective
    float m_surviveTimer = 0.f; // for Survive objective (counts down)
    bool  m_complete     = false;
    bool  m_firePrev     = false;
    int   m_shotCount    = 0;   // for SPLIT_ROUND mod

    // Active program effect timers
    float m_empTimer     = 0.f;
    float m_stealthTimer = 0.f;

    struct InputState {
        bool thrustForward = false;
        bool rotLeft       = false;
        bool rotRight      = false;
        bool fire          = false;
        bool prog0         = false;
        bool prog1         = false;
        bool prog2         = false;
    } m_input;
    bool m_prog0Prev = false, m_prog1Prev = false, m_prog2Prev = false;

    void setupCollisionCallback(SceneContext& ctx);
    void resetGame();
    void handleCollisions();
    void emitDeathParticles();
    void sweepDead();
    void activateProgram(int slot, SceneContext& ctx);
    void checkObjective(SceneContext& ctx);
    void renderICE(SceneContext& ctx) const;
};
