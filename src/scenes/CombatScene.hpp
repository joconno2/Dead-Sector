#pragma once
#include "IScene.hpp"
class AudioSystem;
class SteamManager;
#include "renderer/VectorRenderer.hpp"
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
#include "entities/PhantomICE.hpp"
#include "entities/LeechICE.hpp"
#include "entities/MirrorICE.hpp"
#include "entities/EnemyProjectile.hpp"
#include "entities/PulseMine.hpp"
#include "entities/DataPacket.hpp"
#include "math/Vec2.hpp"
#include <memory>
#include <vector>

struct NodeConfig {
    int           nodeId         = 0;
    int           tier           = 1;
    NodeObjective objective      = NodeObjective::Sweep;
    int           sweepTarget    = 6;
    float         surviveSeconds = 30.f;
    int           extractTarget  = 8;   // data packets to collect (Extract objective)
    float         traceTickRate  = 1.5f;
    bool          endless        = false;
    int           endlessWave    = 0;
    float         startTrace     = 0.f;  // carry trace forward in endless mode
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
    std::vector<std::unique_ptr<PhantomICE>>      m_phantoms;
    std::vector<std::unique_ptr<LeechICE>>        m_leeches;
    std::vector<std::unique_ptr<MirrorICE>>       m_mirrors;
    std::vector<std::unique_ptr<EnemyProjectile>> m_enemyProjectiles;
    std::vector<std::unique_ptr<PulseMine>>       m_mines;
    std::vector<DataPacket>                       m_dataPackets;

    std::vector<std::unique_ptr<HunterICE>>  m_pendingHunters;
    std::vector<std::unique_ptr<Projectile>> m_pendingProjectiles;

    std::unique_ptr<BossEnemy> m_boss;

    int   m_score            = 0;
    float m_scoreMult        = 1.f;
    int   m_iceKilled        = 0;
    int   m_packetsCollected = 0;
    float m_surviveTimer     = 0.f;
    bool  m_complete         = false;
    bool  m_firePrev         = false;
    int   m_shotCount        = 0;
    int   m_lastUpgradeKills = 0;

    float m_bossDeathTimer  = 0.f;  // real-time countdown → NodeComplete transition
    float m_bossPhaseTimer  = 0.f;  // >0 while phase 2 warning is showing
    float m_shakeTimer     = 0.f;  // screen shake duration
    float m_shakeDuration  = 0.f;  // original duration for intensity decay
    float m_thresholdFlash = 0.f;  // 0..1 → fades out, triggers full-screen color pulse
    int   m_thresholdFlashPct = 0; // which threshold was just crossed (25/50/75/100)
    float m_deathTimer     = -1.f; // >0 while waiting to transition to GameOverScene
    Vec2  m_deathPos       = {};   // avatar position at time of death

    float m_empTimer       = 0.f;
    float m_stealthTimer   = 0.f;
    float m_overclockTimer = 0.f;  // OVERCLOCK: cooldown halved while > 0
    float m_baseCdMult     = 1.f;  // computed at node entry; halved by OVERCLOCK
    float m_decoyTimer     = 0.f;  // CLONE: decoy active duration
    Vec2  m_decoyPos       = {};   // CLONE: decoy world-space position
    int   m_scatterCount   = 0;    // SCATTER_CORE: shot counter (splits on 4th)
    bool  m_isGolden       = false; // true when activeHull has a golden clear
    float m_sparkleTimer   = 0.f;  // throttle for gold-ship sparkle particles

    // --- Flashy program effects ---
    struct Drone {
        float orbitAngle;  // radians around avatar
        float fireTimer;   // countdown to next shot
        float life;        // remaining duration
    };
    struct Shockwave {
        Vec2      pos;
        float     radius, maxRadius;
        float     life,   maxLife;
        GlowColor col;
    };
    struct Beam {
        Vec2      from, to;
        float     life, maxLife;
        GlowColor col;
    };
    std::vector<Drone>     m_drones;
    std::vector<Shockwave> m_shockwaves;
    std::vector<Beam>      m_beams;

    struct InputState {
        bool thrustForward = false;
        bool rotLeft       = false;
        bool rotRight      = false;
        bool fire          = false;
        bool prog0 = false, prog1 = false, prog2 = false;
    } m_input;
    bool m_prog0Prev = false, m_prog1Prev = false, m_prog2Prev = false;

    AudioSystem*        m_audio      = nullptr;  // borrowed from SceneContext
    SDL_GameController* m_controller = nullptr;  // borrowed; used for haptic rumble
    SteamManager*       m_steam      = nullptr;  // borrowed; used for achievements

    bool m_firstKillUnlocked = false;  // ACH_FIRST_KILL fired this run
    bool m_highTraceUnlocked = false;  // ACH_HIGH_TRACE fired this run

    // Pause menu
    bool  m_paused       = false;
    int   m_pauseCursor  = 0;   // 0=Resume 1=MusicVol 2=SfxVol 3=QuitToMenu
    float m_pauseTime    = 0.f;
    int   m_pauseMusicVol = 80;
    int   m_pauseSfxVol   = 80;

    void renderPauseOverlay(SceneContext& ctx) const;
    void pauseChangeVolume(int item, int delta, SceneContext& ctx);

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
