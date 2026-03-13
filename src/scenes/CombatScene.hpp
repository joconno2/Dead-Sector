#pragma once
#include "IScene.hpp"
class AudioSystem;
#include "world/Node.hpp"
#include "core/Constants.hpp"
#include "systems/PhysicsSystem.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/TraceSystem.hpp"
#include "systems/SpawnManager.hpp"
#include "systems/AISystem.hpp"
#include "systems/ProgramSystem.hpp"
#include "systems/ParticleSystem.hpp"
#include "systems/FragmentSystem.hpp"
#include "systems/WallSystem.hpp"
#include "entities/Avatar.hpp"
#include "entities/BossEnemy.hpp"
#include "entities/Projectile.hpp"
#include "entities/HunterICE.hpp"
#include "entities/SentryICE.hpp"
#include "entities/SpawnerICE.hpp"
#include "entities/EnemyProjectile.hpp"
#include "entities/PulseMine.hpp"
#include "math/Vec2.hpp"
#include <memory>
#include <vector>

struct NodeConfig {
    int           nodeId         = 0;
    int           tier           = 1;
    NodeObjective objective      = NodeObjective::Sweep;
    int           sweepTarget    = 6;
    float         surviveSeconds = 30.f;
    float         traceTickRate  = 3.0f;
    bool          endless        = false;
    int           endlessWave    = 0;
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
    FragmentSystem  m_fragments;
    WallSystem      m_walls;

    std::unique_ptr<Avatar>                       m_avatar;
    std::vector<std::unique_ptr<Projectile>>      m_projectiles;
    std::vector<std::unique_ptr<HunterICE>>       m_hunters;
    std::vector<std::unique_ptr<SentryICE>>       m_sentries;
    std::vector<std::unique_ptr<SpawnerICE>>      m_spawnerICE;
    std::vector<std::unique_ptr<EnemyProjectile>> m_enemyProjectiles;
    std::vector<std::unique_ptr<PulseMine>>       m_mines;

    std::vector<std::unique_ptr<HunterICE>>  m_pendingHunters;
    std::vector<std::unique_ptr<Projectile>> m_pendingProjectiles;

    std::unique_ptr<BossEnemy> m_boss;

    int   m_score            = 0;
    float m_scoreMult        = 1.f;
    int   m_iceKilled        = 0;
    float m_surviveTimer     = 0.f;
    bool  m_complete         = false;
    bool  m_firePrev         = false;
    int   m_shotCount        = 0;
    int   m_lastUpgradeKills = 0;

    float m_bossDeathTimer = 0.f;  // real-time countdown → NodeComplete transition
    float m_shakeTimer     = 0.f;  // screen shake duration
    float m_shakeDuration  = 0.f;  // original duration for intensity decay

    float m_empTimer     = 0.f;
    float m_stealthTimer = 0.f;

    struct InputState {
        bool thrustForward = false;
        bool rotLeft       = false;
        bool rotRight      = false;
        bool fire          = false;
        bool prog0 = false, prog1 = false, prog2 = false;
    } m_input;
    bool m_prog0Prev = false, m_prog1Prev = false, m_prog2Prev = false;

    AudioSystem* m_audio = nullptr;  // borrowed from SceneContext

    void setupCollisionCallback(SceneContext& ctx);
    void resetGame(SceneContext& ctx);
    void handleCollisions();
    void emitDeathParticles();
    void emitDeathFragments();
    void sweepDead();
    void activateProgram(int slot, SceneContext& ctx);
    void checkObjective(SceneContext& ctx);
    void renderICE(SceneContext& ctx) const;
    void handleRicochet(bool hasRicochet);
    void updateMines(float dt, SceneContext& ctx);
    void renderMines(SceneContext& ctx) const;
    void spawnMines(int tier, int nodeId);
};
