#include "CombatScene.hpp"
#include "audio/AudioSystem.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "GameOverScene.hpp"
#include "NodeCompleteScene.hpp"
#include "RunSummaryScene.hpp"
#include "VictoryScene.hpp"
#include "WorldCompleteScene.hpp"
#include "core/Constants.hpp"
#include "core/SaveSystem.hpp"
#include "core/Worlds.hpp"
#include "systems/ModSystem.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "renderer/BuildChart.hpp"
#include "math/Vec2.hpp"

#include <SDL.h>
#include <SDL_mixer.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

CombatScene::CombatScene(NodeConfig cfg) : m_config(cfg) {}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void CombatScene::onEnter(SceneContext& ctx) {
    // Cooldown mult: mod-based × shop COOL_EXEC stacks
    {
        m_baseCdMult = ctx.mods ? ctx.mods->cdMult() : 1.f;
        if (ctx.saveData) {
            int stacks = ctx.saveData->purchaseCount("COOL_EXEC");
            for (int i = 0; i < stacks; ++i) m_baseCdMult *= 0.90f;
        }
        if (ctx.programs) {
            ctx.programs->resetCooldowns();
            ctx.programs->setCooldownMultiplier(m_baseCdMult);
        }
    }

    // Score multiplier: shop SCORE_BOOST (+20% per stack)
    m_scoreMult = 1.f;
    if (ctx.saveData) {
        int stacks = ctx.saveData->purchaseCount("SCORE_BOOST");
        for (int i = 0; i < stacks; ++i) m_scoreMult *= 1.20f;
    }

    m_audio = ctx.audio;
    if (m_audio)
        m_audio->playMusic("assets/music/Karl Casey - Fortress.mp3");

    m_paused      = false;
    m_pauseCursor = 0;
    m_pauseTime   = 0.f;
    m_pauseMusicVol = ctx.saveData ? ctx.saveData->musicVolume : 80;
    m_pauseSfxVol   = ctx.saveData ? ctx.saveData->sfxVolume  : 80;

    setupCollisionCallback(ctx);
    resetGame(ctx);

    // Extra lives: mods + shop EXTRA_LIFE stacks + Event node bonus
    if (m_avatar) {
        int lives = ctx.mods ? ctx.mods->startingExtraLives() : 0;
        if (ctx.saveData) lives += ctx.saveData->purchaseCount("EXTRA_LIFE");
        lives += ctx.bonusLives;
        ctx.bonusLives = 0;
        m_avatar->extraLives += lives; // hull base lives already set in resetGame
    }

    // Trace tick rate: base × difficulty × TRACE_SLOW shop stacks × Ghost Protocol mod
    {
        float tickRate = m_config.traceTickRate;
        // Difficulty multiplier: 0=Runner(0.65), 1=Decker(1.0), 2=Netrunner(1.6)
        static const float DIFF_MULT[3] = { 0.65f, 1.0f, 1.6f };
        int diff = std::max(0, std::min(2, ctx.difficulty));
        tickRate *= DIFF_MULT[diff];
        if (ctx.saveData) {
            int stacks = ctx.saveData->purchaseCount("TRACE_SLOW");
            for (int i = 0; i < stacks; ++i) tickRate *= 0.90f;
        }
        if (ctx.mods && ctx.mods->hasGhostProtocol()) tickRate *= 0.6f;
        m_trace.setTickRate(tickRate);
    }

    // Threshold alarm SFX + flash
    m_trace.setThresholdCallback([this](int pct) {
        if (m_audio) m_audio->playThreshold();
        m_thresholdFlash    = 1.f;
        m_thresholdFlashPct = pct;
    });
}

void CombatScene::onExit() {
    if (m_audio) m_audio->stopMusic();
    m_audio = nullptr;
    m_projectiles.clear();
    m_hunters.clear();
    m_sentries.clear();
    m_spawnerICE.clear();
    m_phantoms.clear();
    m_leeches.clear();
    m_mirrors.clear();
    m_enemyProjectiles.clear();
    m_pendingHunters.clear();
    m_pendingProjectiles.clear();
    m_mines.clear();
    m_avatar.reset();
}

void CombatScene::setupCollisionCallback(SceneContext& ctx) {
    SceneManager*    scenes     = ctx.scenes;
    ModSystem*       mods       = ctx.mods;
    bool*            invincible = &ctx.debugInvincible;
    const SaveData*  saveData   = ctx.saveData;
    m_collision.setCallback([this, scenes, mods, invincible, saveData](const CollisionEvent& ev) {
        if (ev.type == CollisionEvent::Type::ProjectileHitICE) {
            if (!ev.projectile->alive || !ev.iceEntity->alive) return;

            bool pierce = ev.projectile->pierce;  // BREACH program
            if (!pierce && mods) {
                if (mods->hasPhantomRound()) pierce = true;
                else if (mods->hasCritMatrix()) {
                    static thread_local std::mt19937 rng(std::random_device{}());
                    std::uniform_real_distribution<float> d(0.f, 1.f);
                    if (d(rng) < 0.30f) pierce = true;
                }
            }
            if (!pierce) ev.projectile->alive = false;
            Vec2 killPos = ev.iceEntity->pos;
            ev.iceEntity->alive = false;
            m_score += static_cast<int>(ev.iceEntity->scoreValue() * m_scoreMult);
            m_iceKilled++;
            if (m_config.objective == NodeObjective::Extract) {
                static thread_local std::mt19937 dropRng(std::random_device{}());
                std::uniform_real_distribution<float> ang(0.f, 6.28318f);
                std::uniform_real_distribution<float> spd(20.f, 60.f);
                float a = ang(dropRng);
                DataPacket dp;
                dp.pos = killPos;
                dp.vel = { std::cos(a) * spd(dropRng), std::sin(a) * spd(dropRng) };
                m_dataPackets.push_back(dp);
            }
            if (m_audio) m_audio->playExplosion();

            if (mods) m_trace.add(-mods->traceSinkAmt());

            if (mods && mods->hasPhaseFrame() && m_avatar && m_avatar->alive) {
                m_avatar->shieldTimer = std::max(m_avatar->shieldTimer, 0.5f);
                m_avatar->shielded    = true;
            }

            if (mods && mods->hasNovaBurst()) {
                for (int k = 0; k < 4; ++k) {
                    float a   = k * (6.28318f / 4.f);
                    Vec2  dir = Vec2::fromAngle(a);
                    m_pendingProjectiles.emplace_back(
                        std::make_unique<Projectile>(killPos, dir * Constants::PROJ_SPEED, a));
                }
            }

            if (mods && mods->hasChainFire()) {
                Entity* nearest = nullptr; float best = std::numeric_limits<float>::max();
                auto checkNearest = [&](auto& vec) {
                    for (auto& e : vec)
                        if (e->alive) {
                            float d = (e->pos - killPos).length();
                            if (d < best) { best = d; nearest = e.get(); }
                        }
                };
                checkNearest(m_hunters); checkNearest(m_sentries); checkNearest(m_spawnerICE); checkNearest(m_phantoms); checkNearest(m_leeches); checkNearest(m_mirrors);
                if (nearest) {
                    Vec2 diff = (nearest->pos - killPos).normalized();
                    float a = std::atan2(diff.y, diff.x);
                    m_pendingProjectiles.emplace_back(
                        std::make_unique<Projectile>(killPos, diff * Constants::PROJ_SPEED, a));
                }
            }

            // OVERLOAD_COIL: killing a SpawnerICE detonates a 150px AoE
            if (mods && mods->hasOverloadCoil()) {
                bool wasSpawner = false;
                for (auto& sp : m_spawnerICE)
                    if (sp.get() == ev.iceEntity) { wasSpawner = true; break; }
                if (wasSpawner) {
                    for (auto& e : m_hunters)
                        if (e->alive && (e->pos - killPos).length() <= 150.f) {
                            e->alive = false;
                            m_score += static_cast<int>(e->scoreValue() * m_scoreMult);
                            m_iceKilled++;
                        }
                    for (auto& e : m_sentries)
                        if (e->alive && (e->pos - killPos).length() <= 150.f) {
                            e->alive = false;
                            m_score += static_cast<int>(e->scoreValue() * m_scoreMult);
                            m_iceKilled++;
                        }
                }
            }

        } else if (ev.type == CollisionEvent::Type::AvatarHitICE) {
            if (!ev.avatar->alive) return;
            if (ev.avatar->shielded || *invincible) { ev.iceEntity->alive = false; return; }
            if (ev.avatar->extraLives > 0) {
                ev.iceEntity->alive = false;
                ev.avatar->extraLives--;
                ev.avatar->shieldTimer = 0.8f;
                ev.avatar->shielded    = true;
                m_trace.onHit();
                if (mods && mods->hasReactivePlating())
                    for (auto& e : m_hunters)   if (e->alive && (e->pos-ev.avatar->pos).length()<=150.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                return;
            }
            if (mods && mods->hasReactivePlating()) {
                for (auto& e : m_hunters)   if (e->alive && (e->pos-ev.avatar->pos).length()<=150.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_sentries)  if (e->alive && (e->pos-ev.avatar->pos).length()<=150.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_spawnerICE)if (e->alive && (e->pos-ev.avatar->pos).length()<=150.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_phantoms)   if (e->alive && (e->pos-ev.avatar->pos).length()<=150.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_leeches)    if (e->alive && (e->pos-ev.avatar->pos).length()<=150.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_mirrors)    if (e->alive && (e->pos-ev.avatar->pos).length()<=150.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
            }
            ev.avatar->alive    = false;
            ev.iceEntity->alive = false;
            m_trace.onHit();
            // DEADMAN_SWITCH: kill all ICE within 200px on death
            if (mods && mods->hasDeadmanSwitch()) {
                Vec2 dp = ev.avatar->pos;
                for (auto& e : m_hunters)    if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_sentries)   if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_spawnerICE) if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_phantoms)   if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_leeches)    if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_mirrors)    if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
            }
            m_deathPos = ev.avatar->pos;
            m_deathTimer = 0.65f;
            m_shakeTimer = m_shakeDuration = 0.5f;
            m_fragments.emit(m_deathPos, Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 18, 16.f, 380.f);
            m_particles.emit(m_deathPos,  Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 24);

        } else if (ev.type == CollisionEvent::Type::EnemyProjectileHitAvatar) {
            if (!ev.avatar->alive || !ev.enemyProj->alive) return;
            if (ev.avatar->shielded || *invincible) { ev.enemyProj->alive = false; return; }
            if (ev.avatar->extraLives > 0) {
                ev.enemyProj->alive = false;
                ev.avatar->extraLives--;
                ev.avatar->shieldTimer = 0.8f;
                ev.avatar->shielded    = true;
                m_trace.onHit();
                return;
            }
            ev.enemyProj->alive = false;
            ev.avatar->alive    = false;
            m_trace.onHit();
            // DEADMAN_SWITCH: kill all ICE within 200px on death
            if (mods && mods->hasDeadmanSwitch()) {
                Vec2 dp = ev.avatar->pos;
                for (auto& e : m_hunters)    if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_sentries)   if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_spawnerICE) if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_phantoms)   if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_leeches)    if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_mirrors)    if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
            }
            m_deathPos = ev.avatar->pos;
            m_deathTimer = 0.65f;
            m_shakeTimer = m_shakeDuration = 0.5f;
            m_fragments.emit(m_deathPos, Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 18, 16.f, 380.f);
            m_particles.emit(m_deathPos,  Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 24);

        } else if (ev.type == CollisionEvent::Type::ProjectileHitAvatar) {
            if (!ev.avatar->alive || !ev.projectile->alive) return;
            // DEADBOLT PROTOCOL upgrade grants immunity to own bullets
            bool immune = saveData && saveData->purchaseCount("DEADBOLT") > 0;
            if (immune || ev.avatar->shielded || *invincible) {
                ev.projectile->alive = false;
                return;
            }
            ev.projectile->alive = false;
            if (ev.avatar->extraLives > 0) {
                ev.avatar->extraLives--;
                ev.avatar->shieldTimer = 0.8f;
                ev.avatar->shielded    = true;
                m_trace.onHit();
                return;
            }
            ev.avatar->alive = false;
            m_trace.onHit();
            if (mods && mods->hasDeadmanSwitch()) {
                Vec2 dp = ev.avatar->pos;
                for (auto& e : m_hunters)    if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_sentries)   if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_spawnerICE) if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_phantoms)   if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_leeches)    if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
                for (auto& e : m_mirrors)    if (e->alive && (e->pos-dp).length()<=200.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
            }
            m_deathPos = ev.avatar->pos;
            m_deathTimer = 0.65f;
            m_shakeTimer = m_shakeDuration = 0.5f;
            m_fragments.emit(m_deathPos, Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 18, 16.f, 380.f);
            m_particles.emit(m_deathPos,  Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 24);
        }
    });
}

void CombatScene::resetGame(SceneContext& ctx) {
    m_score             = 0;
    m_iceKilled         = 0;
    m_packetsCollected  = 0;
    m_complete          = false;
    m_firePrev          = false;
    m_prog0Prev = m_prog1Prev = m_prog2Prev = false;
    m_empTimer          = 0.f;
    m_stealthTimer      = 0.f;
    m_overclockTimer    = 0.f;
    m_decoyTimer        = 0.f;
    m_scatterCount      = 0;
    m_surviveTimer      = m_config.surviveSeconds;
    m_lastUpgradeKills  = 0;
    m_bossDeathTimer    = 0.f;
    m_bossPhaseTimer    = 0.f;
    m_shakeTimer        = 0.f;
    m_shakeDuration     = 0.f;
    m_thresholdFlash    = 0.f;
    m_deathTimer        = -1.f;
    m_boss.reset();

    m_projectiles.clear();
    m_hunters.clear();
    m_sentries.clear();
    m_spawnerICE.clear();
    m_phantoms.clear();
    m_leeches.clear();
    m_mirrors.clear();
    m_enemyProjectiles.clear();
    m_pendingHunters.clear();
    m_pendingProjectiles.clear();
    m_mines.clear();
    m_dataPackets.clear();

    m_particles.clear();
    m_fragments.clear();
    m_trace.reset();
    if (m_config.startTrace > 0.f) m_trace.add(m_config.startTrace);
    m_trace.setTickRate(m_config.traceTickRate);
    m_spawner.reset();

    // Hull selection from save
    HullType hull = HullType::Delta;
    if (ctx.saveData) hull = hullFromString(ctx.saveData->activeHull);

    m_avatar = std::make_unique<Avatar>(
        Constants::SCREEN_WF * 0.5f, Constants::SCREEN_HF * 0.5f, hull);

    // Apply hull-specific base extra lives (mod/shop bonuses added in onEnter)
    if (m_avatar)
        m_avatar->extraLives = m_avatar->hullStats.extraLives;

    // Generate walls (not for endless — walls are per-node, seeded by nodeId)
    if (!m_config.endless && m_config.nodeId >= 0) {
        m_walls.generate(m_config.tier, m_config.nodeId);
        spawnMines(m_config.tier, m_config.nodeId);
    } else {
        m_walls.clear();
    }

    // Spawn boss for Boss objective nodes — type determined by current world
    if (m_config.objective == NodeObjective::Boss) {
        static thread_local std::mt19937 rng(std::random_device{}());
        BossType type = static_cast<BossType>(worldDef(ctx.currentWorld).bossType);

        // Pick a random angle and place the boss ~220px from centre (player spawns at centre)
        std::uniform_real_distribution<float> aDist(0.f, 6.28318f);
        float angle = aDist(rng);
        constexpr float OFFSET = 220.f;
        const float cx = Constants::SCREEN_WF * 0.5f;
        const float cy = Constants::SCREEN_HF * 0.5f;
        Vec2 bossPos = {
            std::max(80.f, std::min(cx + std::cos(angle) * OFFSET, Constants::SCREEN_WF - 80.f)),
            std::max(80.f, std::min(cy + std::sin(angle) * OFFSET, Constants::SCREEN_HF - 80.f))
        };
        m_boss = std::make_unique<BossEnemy>(type, bossPos);
    }

    // Endless mode: spawn a boss every 5 waves (waves 5, 10, 15…)
    if (m_config.endless && m_config.endlessWave > 0 && m_config.endlessWave % 5 == 0) {
        static thread_local std::mt19937 rng2(std::random_device{}());
        // Cycle through the 3 boss types based on wave
        BossType endlessBossType = static_cast<BossType>((m_config.endlessWave / 5 - 1) % 3);
        std::uniform_real_distribution<float> aDist2(0.f, 6.28318f);
        float angle2 = aDist2(rng2);
        constexpr float OFFSET2 = 220.f;
        const float cx2 = Constants::SCREEN_WF * 0.5f;
        const float cy2 = Constants::SCREEN_HF * 0.5f;
        Vec2 bossPos2 = {
            std::max(80.f, std::min(cx2 + std::cos(angle2) * OFFSET2, Constants::SCREEN_WF - 80.f)),
            std::max(80.f, std::min(cy2 + std::sin(angle2) * OFFSET2, Constants::SCREEN_HF - 80.f))
        };
        m_boss = std::make_unique<BossEnemy>(endlessBossType, bossPos2);
    }
}

// ---------------------------------------------------------------------------
// Mine spawning
// ---------------------------------------------------------------------------

void CombatScene::spawnMines(int tier, int nodeId) {
    // Mines per tier: 0, 0, 1-2, 2-3  (tiers 1-4)
    if (tier < 3) return;

    std::mt19937 rng(static_cast<uint32_t>(nodeId * 7919 + 4321));
    std::uniform_int_distribution<int> countDist(tier - 2, tier - 1); // tier3→1-2, tier4→2-3
    int count = countDist(rng);

    const float W = Constants::SCREEN_WF;
    const float H = Constants::SCREEN_HF;
    std::uniform_real_distribution<float> xDist(60.f, W - 60.f);
    std::uniform_real_distribution<float> yDist(60.f, H - 60.f);

    for (int i = 0; i < count; ++i) {
        for (int attempt = 0; attempt < 20; ++attempt) {
            float mx = xDist(rng), my = yDist(rng);
            // Keep away from spawn center
            float dx = mx - W * 0.5f, dy = my - H * 0.5f;
            if (std::sqrt(dx*dx + dy*dy) < 160.f) continue;
            // Keep away from other mines
            bool tooClose = false;
            for (const auto& m : m_mines) {
                float ddx = m->pos.x - mx, ddy = m->pos.y - my;
                if (std::sqrt(ddx*ddx+ddy*ddy) < 100.f) { tooClose = true; break; }
            }
            if (tooClose) continue;
            m_mines.emplace_back(std::make_unique<PulseMine>(mx, my));
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void CombatScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    // Toggle pause on ESC / Start
    bool togglePause = false;
    if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
        togglePause = true;
    else if (ev.type == SDL_CONTROLLERBUTTONDOWN &&
             ev.cbutton.button == SDL_CONTROLLER_BUTTON_START)
        togglePause = true;

    if (togglePause) {
        m_paused = !m_paused;
        if (!m_paused) { // on resume, save any volume changes
            if (ctx.saveData) {
                ctx.saveData->musicVolume = m_pauseMusicVol;
                ctx.saveData->sfxVolume   = m_pauseSfxVol;
                SaveSystem::save(*ctx.saveData);
            }
        }
        return;
    }

    if (!m_paused) return;

    // Pause menu navigation
    constexpr int PAUSE_ITEMS = 4; // Resume, MusicVol, SfxVol, QuitToMenu
    bool up = false, down = false, left = false, right = false, confirm = false;

    if (ev.type == SDL_KEYDOWN) {
        switch (ev.key.keysym.sym) {
        case SDLK_UP:   case SDLK_w: up      = true; break;
        case SDLK_DOWN: case SDLK_s: down    = true; break;
        case SDLK_LEFT: case SDLK_a: left    = true; break;
        case SDLK_RIGHT:case SDLK_d: right   = true; break;
        case SDLK_RETURN: case SDLK_SPACE: confirm = true; break;
        default: break;
        }
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (ev.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:    up      = true; break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  down    = true; break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  left    = true; break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: right   = true; break;
        case SDL_CONTROLLER_BUTTON_A:          confirm = true; break;
        case SDL_CONTROLLER_BUTTON_B:          // B = resume
            m_paused = false;
            if (ctx.saveData) {
                ctx.saveData->musicVolume = m_pauseMusicVol;
                ctx.saveData->sfxVolume   = m_pauseSfxVol;
                SaveSystem::save(*ctx.saveData);
            }
            return;
        default: break;
        }
    }

    if (up)   m_pauseCursor = (m_pauseCursor - 1 + PAUSE_ITEMS) % PAUSE_ITEMS;
    if (down) m_pauseCursor = (m_pauseCursor + 1) % PAUSE_ITEMS;
    if (left || right) pauseChangeVolume(m_pauseCursor, left ? -1 : +1, ctx);

    if (confirm) {
        if (m_pauseCursor == 0) { // Resume
            m_paused = false;
            if (ctx.saveData) {
                ctx.saveData->musicVolume = m_pauseMusicVol;
                ctx.saveData->sfxVolume   = m_pauseSfxVol;
                SaveSystem::save(*ctx.saveData);
            }
        } else if (m_pauseCursor == 3) { // Quit to menu
            if (ctx.saveData) {
                ctx.saveData->musicVolume = m_pauseMusicVol;
                ctx.saveData->sfxVolume   = m_pauseSfxVol;
            }
            ctx.scenes->replace(std::make_unique<RunSummaryScene>(
                m_score, m_iceKilled, ctx.runNodes, false));
        }
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void CombatScene::update(float dt, SceneContext& ctx) {
    if (m_complete) return;
    if (m_paused) { m_pauseTime += dt; return; }

    // Avatar death animation timer
    if (m_deathTimer > 0.f) {
        m_deathTimer -= dt;
        m_particles.update(dt);
        m_fragments.update(dt);
        if (m_shakeTimer > 0.f) m_shakeTimer -= dt;
        if (m_deathTimer <= 0.f)
            ctx.scenes->replace(std::make_unique<GameOverScene>(m_score));
        return;
    }

    // Real-time meta effects (shake, slow-mo, boss death transition)
    const float realDt = dt;
    if (m_shakeTimer > 0.f) m_shakeTimer -= realDt;
    if (m_bossDeathTimer > 0.f) {
        m_bossDeathTimer -= realDt;
        dt *= 0.10f;   // slow-mo during boss death sequence
        if (m_bossDeathTimer <= 0.f) {
            // Endless mode: boss was a bonus encounter — resume the wave, don't end
            if (m_config.endless) {
                m_boss.reset();
                return;
            }
            m_complete = true;
            ctx.runNodes++;
            if (ctx.currentWorld < 2) {
                // Advance to the next world
                ctx.scenes->replace(std::make_unique<WorldCompleteScene>(
                    ctx.currentWorld, m_score, m_iceKilled));
            } else {
                // Final world beaten — true victory
                HullType hull = HullType::Delta;
                if (ctx.saveData) hull = hullFromString(ctx.saveData->activeHull);
                ctx.scenes->replace(std::make_unique<VictoryScene>(
                    m_score, m_iceKilled, ctx.runNodes, hull));
            }
            return;
        }
    }

    const bool hasRicochetMod = ctx.mods && ctx.mods->hasRicochet();

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    m_input.thrustForward = keys[SDL_SCANCODE_UP]   || keys[SDL_SCANCODE_W];
    m_input.rotLeft       = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
    m_input.rotRight      = keys[SDL_SCANCODE_RIGHT]|| keys[SDL_SCANCODE_D];
    m_input.fire          = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]
                          || keys[SDL_SCANCODE_LCTRL];
    m_input.prog0 = keys[SDL_SCANCODE_Q];
    m_input.prog1 = keys[SDL_SCANCODE_E];
    m_input.prog2 = keys[SDL_SCANCODE_R];

    if (ctx.controller) {
        constexpr Sint16 DEAD = 8000;
        Sint16 lx = SDL_GameControllerGetAxis(ctx.controller, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 rt = SDL_GameControllerGetAxis(ctx.controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        if (lx < -DEAD || SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  m_input.rotLeft  = true;
        if (lx >  DEAD || SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) m_input.rotRight = true;
        if (SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_A) ||
            SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_DPAD_UP))
            m_input.thrustForward = true;
        if (SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) || rt > DEAD)
            m_input.fire = true;
        if (SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_X)) m_input.prog0 = true;
        if (SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_Y)) m_input.prog1 = true;
        if (SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_B)) m_input.prog2 = true;
    }

    const HullStats& hs = m_avatar ? m_avatar->hullStats : statsForHull(HullType::Delta);
    float thrustMult  = hs.thrustMult    * (ctx.mods ? ctx.mods->thrustMult()    : 1.f);
    float rotMult     = hs.rotMult       * (ctx.mods ? ctx.mods->rotMult()       : 1.f);
    float speedCapMult= hs.speedMult     * (ctx.mods ? ctx.mods->maxSpeedMult()  : 1.f);
    float pSpeedMult  = hs.projSpeedMult * (ctx.mods ? ctx.mods->projSpeedMult() : 1.f);
    float pRadMult    = ctx.mods ? ctx.mods->projRadiusMult(): 1.f;
    bool  splitRound  = ctx.mods ? ctx.mods->hasSplitRound()  : false;
    bool  overcharge  = ctx.mods ? ctx.mods->hasOvercharge()   : false;
    bool  scatterCore = ctx.mods ? ctx.mods->hasScatterCore()  : false;

    if (m_avatar && m_avatar->alive) {
        if (m_input.rotLeft)       m_avatar->rotateLeft(dt  * rotMult);
        if (m_input.rotRight)      m_avatar->rotateRight(dt * rotMult);
        if (m_input.thrustForward) m_avatar->applyThrust(dt, thrustMult, speedCapMult);

        if (m_input.thrustForward) {
            Vec2 heading    = Vec2::fromAngle(m_avatar->angle);
            Vec2 perp       = { -heading.y, heading.x };
            Vec2 exhaustDir = { -heading.x, -heading.y };
            Vec2 eR = m_avatar->pos - heading * 25.f + perp * 3.f;
            Vec2 eL = m_avatar->pos - heading * 25.f - perp * 3.f;
            m_particles.emitThrust(eR, exhaustDir, 80, 200, 255, 3);
            m_particles.emitThrust(eL, exhaustDir, 80, 200, 255, 3);
        }

        bool rapidFire = (m_avatar->overdriveTimer > 0.f);
        bool fireEdge  = m_input.fire && (!m_firePrev || rapidFire);
        if (fireEdge) {
            Projectile* p = m_avatar->fire(pSpeedMult, pRadMult);
            if (p) {
                if (hasRicochetMod) p->noWrap = true;
                m_projectiles.emplace_back(p);
                m_shotCount++;
                m_scatterCount++;
                if (m_audio) m_audio->playShot();

                if (splitRound && m_shotCount % 3 == 0) {
                    Vec2 h    = Vec2::fromAngle(m_avatar->angle);
                    Vec2 perp = { -h.y, h.x };
                    Vec2 twinPos = m_avatar->pos + h * 18.f + perp * 10.f;
                    auto twin = std::make_unique<Projectile>(twinPos, h * (Constants::PROJ_SPEED * pSpeedMult), m_avatar->angle);
                    twin->radius *= pRadMult;
                    if (hasRicochetMod) twin->noWrap = true;
                    m_projectiles.push_back(std::move(twin));
                }

                if (overcharge && m_shotCount % 5 == 0) {
                    constexpr float offsets[] = { -0.28f, 0.f, 0.28f };
                    for (float off : offsets) {
                        float a   = m_avatar->angle + off;
                        Vec2 dir  = Vec2::fromAngle(a);
                        Vec2 bpos = m_avatar->pos + dir * 18.f;
                        auto burst = std::make_unique<Projectile>(bpos, dir * (Constants::PROJ_SPEED * pSpeedMult), a);
                        burst->radius *= pRadMult;
                        if (hasRicochetMod) burst->noWrap = true;
                        m_projectiles.push_back(std::move(burst));
                    }
                }

                // SCATTER_CORE: every 4th shot fires 2 additional spread shots
                if (scatterCore && m_scatterCount % 4 == 0) {
                    constexpr float SCATTER_SPREAD[] = { -0.436f, 0.436f };  // ±25°
                    for (float off : SCATTER_SPREAD) {
                        float a   = m_avatar->angle + off;
                        Vec2  dir = Vec2::fromAngle(a);
                        Vec2  spos = m_avatar->pos + dir * 16.f;
                        auto  sp2 = std::make_unique<Projectile>(spos, dir * (Constants::PROJ_SPEED * pSpeedMult), a);
                        sp2->radius *= pRadMult;
                        if (hasRicochetMod) sp2->noWrap = true;
                        m_projectiles.push_back(std::move(sp2));
                    }
                }
            }
        }
        m_firePrev = m_input.fire;

        if (m_input.prog0 && !m_prog0Prev) activateProgram(0, ctx);
        if (m_input.prog1 && !m_prog1Prev) activateProgram(1, ctx);
        if (m_input.prog2 && !m_prog2Prev) activateProgram(2, ctx);
    }
    m_prog0Prev = m_input.prog0;
    m_prog1Prev = m_input.prog1;
    m_prog2Prev = m_input.prog2;

    if (ctx.programs) ctx.programs->update(dt);
    if (m_empTimer         > 0.f) m_empTimer         -= dt;
    if (m_stealthTimer     > 0.f) m_stealthTimer     -= dt;
    if (m_thresholdFlash   > 0.f) m_thresholdFlash   -= dt * 1.8f;
    if (m_bossPhaseTimer   > 0.f) m_bossPhaseTimer   -= dt;
    if (m_overclockTimer > 0.f) {
        m_overclockTimer -= dt;
        if (ctx.programs) ctx.programs->setCooldownMultiplier(m_baseCdMult * 0.5f);
        if (m_overclockTimer <= 0.f && ctx.programs)
            ctx.programs->setCooldownMultiplier(m_baseCdMult);
    }
    if (m_decoyTimer > 0.f) m_decoyTimer -= dt;

    // Physics
    std::vector<Entity*> all;
    all.reserve(1 + m_projectiles.size() + m_hunters.size()
                  + m_sentries.size() + m_spawnerICE.size() + m_phantoms.size() + m_leeches.size()
                  + m_mirrors.size() + m_enemyProjectiles.size());
    if (m_avatar && m_avatar->alive) all.push_back(m_avatar.get());
    for (auto& p  : m_projectiles)      all.push_back(p.get());
    for (auto& h  : m_hunters)          all.push_back(h.get());
    for (auto& s  : m_sentries)         all.push_back(s.get());
    for (auto& sp : m_spawnerICE)       all.push_back(sp.get());
    for (auto& ph : m_phantoms)         all.push_back(ph.get());
    for (auto& lc : m_leeches)          all.push_back(lc.get());
    for (auto& mi : m_mirrors)          all.push_back(mi.get());
    for (auto& ep : m_enemyProjectiles) all.push_back(ep.get());
    m_physics.update(dt, all);

    // Wall collisions
    if (!m_walls.empty()) {
        // Avatar bounces off walls
        if (m_avatar && m_avatar->alive)
            m_walls.resolveCircle(m_avatar->pos, m_avatar->vel, m_avatar->radius, 0.6f);

        // Projectiles bounce off walls (always — walls are tactically reflective)
        for (auto& p : m_projectiles) {
            if (!p->alive) continue;
            bool wasNoWrap = p->noWrap;
            p->noWrap = true; // temporarily prevent wrap during bounce
            if (m_walls.resolveCircle(p->pos, p->vel, p->radius, 0.9f)) {
                if (!wasNoWrap) p->noWrap = false;
                // don't mark bounced — wall bounces are always allowed
            } else {
                p->noWrap = wasNoWrap;
            }
        }

        // Enemy projectiles bounce off walls too
        for (auto& ep : m_enemyProjectiles)
            if (ep->alive) m_walls.resolveCircle(ep->pos, ep->vel, ep->radius, 0.9f);

        // ICE entities bounce off walls (walls act as cover)
        for (auto& h  : m_hunters)
            if (h->alive)  m_walls.resolveCircle(h->pos,  h->vel,  h->radius,  0.75f);
        for (auto& s  : m_sentries)
            if (s->alive)  m_walls.resolveCircle(s->pos,  s->vel,  s->radius,  0.75f);
        for (auto& sp : m_spawnerICE)
            if (sp->alive) m_walls.resolveCircle(sp->pos, sp->vel, sp->radius, 0.75f);
        for (auto& ph : m_phantoms)
            if (ph->alive) m_walls.resolveCircle(ph->pos, ph->vel, ph->radius, 0.75f);
        for (auto& lc : m_leeches)
            if (lc->alive && !lc->attached) m_walls.resolveCircle(lc->pos, lc->vel, lc->radius, 0.75f);
        for (auto& mi : m_mirrors)
            if (mi->alive) m_walls.resolveCircle(mi->pos, mi->vel, mi->radius, 0.75f);
    }

    // RICOCHET: bounce player projectiles off screen edges
    if (hasRicochetMod) handleRicochet(true);

    // AI
    if (m_empTimer <= 0.f) {
        AIConfig aiCfg;
        aiCfg.decoyActive        = (m_decoyTimer > 0.f);
        aiCfg.decoyPos           = m_decoyPos;
        aiCfg.sentryFireRateMult = ctx.mods ? ctx.mods->sentryFireRateMult() : 1.f;

        auto aiResult = m_ai.update(dt, m_hunters, m_sentries, m_spawnerICE, m_phantoms, m_leeches, m_mirrors, m_avatar.get(), aiCfg);
        for (auto& ep : aiResult.firedProjectiles)
            m_enemyProjectiles.push_back(std::move(ep));
        for (auto& h : aiResult.spawnedHunters)
            m_pendingHunters.push_back(std::move(h));
        // Leech trace drain
        if (aiResult.leechTraceGain > 0.f) m_trace.add(aiResult.leechTraceGain);
    }

    // Pulse mines
    updateMines(dt, ctx);

    handleCollisions();

    // Projectile vs mine collision
    for (auto& p : m_projectiles) {
        if (!p->alive) continue;
        for (auto& mine : m_mines) {
            if (!mine->alive) continue;
            if ((p->pos - mine->pos).length() < mine->radius + p->radius) {
                p->alive = false;
                mine->hitsLeft--;
                if (mine->hitsLeft <= 0) {
                    mine->alive = false;
                    m_score += static_cast<int>(mine->scoreValue() * m_scoreMult);
                    m_fragments.emit(mine->pos, 255, 140, 20, 10, 14.f, 260.f);
                }
                break;
            }
        }
    }

    for (auto& h : m_pendingHunters) m_hunters.push_back(std::move(h));
    m_pendingHunters.clear();
    for (auto& p : m_pendingProjectiles) m_projectiles.push_back(std::move(p));
    m_pendingProjectiles.clear();

    emitDeathParticles();
    emitDeathFragments();
    sweepDead();
    m_mines.erase(std::remove_if(m_mines.begin(), m_mines.end(),
                  [](const auto& m){ return !m->alive; }), m_mines.end());

    m_particles.update(dt);
    m_fragments.update(dt);

    if (m_stealthTimer <= 0.f) m_trace.update(dt);

    int liveCount = (int)(m_hunters.size() + m_sentries.size() + m_spawnerICE.size() + m_phantoms.size() + m_leeches.size() + m_mirrors.size());
    // Endless wave 10+: ensure advanced ICE types (Phantom/Leech/Mirror) can spawn
    float spawnTrace = m_trace.trace();
    if (m_config.endless && m_config.endlessWave >= 10)
        spawnTrace = std::max(spawnTrace, 75.f);
    auto batch = m_spawner.update(dt, spawnTrace, liveCount);
    for (auto& h  : batch.hunters)    m_hunters.push_back(std::move(h));
    for (auto& s  : batch.sentries)   m_sentries.push_back(std::move(s));
    for (auto& sp : batch.spawnerICE) m_spawnerICE.push_back(std::move(sp));
    for (auto& ph : batch.phantoms)   m_phantoms.push_back(std::move(ph));
    for (auto& lc : batch.leeches)    m_leeches.push_back(std::move(lc));
    for (auto& mi : batch.mirrors)    m_mirrors.push_back(std::move(mi));

    if (m_config.objective == NodeObjective::Survive) m_surviveTimer -= dt;

    // Mirror ICE — reflect front-hit projectiles, take damage from rear hits
    for (auto& mi : m_mirrors) {
        if (!mi->alive) continue;
        for (auto& p : m_projectiles) {
            if (!p->alive) continue;
            float dist = (p->pos - mi->pos).length();
            if (dist >= mi->radius + p->radius) continue;
            // Dot product: facing direction vs vector from mirror to projectile
            // facingAngle points toward the player, so the face is at +facingAngle
            Vec2 faceDir = { std::cos(mi->facingAngle), std::sin(mi->facingAngle) };
            Vec2 toProj  = (p->pos - mi->pos).normalized();
            float dot    = faceDir.x * toProj.x + faceDir.y * toProj.y;
            if (dot > 0.f) {
                // Front hit — reflect: bounce velocity off face normal
                Vec2 norm   = faceDir;
                float vDot  = p->vel.x * norm.x + p->vel.y * norm.y;
                Vec2 reflVel = { p->vel.x - 2.f * vDot * norm.x,
                                 p->vel.y - 2.f * vDot * norm.y };
                p->alive = false;
                auto ep = std::make_unique<EnemyProjectile>(p->pos, reflVel);
                ep->lifetime = 2.0f;
                m_enemyProjectiles.push_back(std::move(ep));
                m_fragments.emit(p->pos, 200, 230, 255, 4, 8.f, 160.f);
            } else {
                // Rear hit — damage the mirror
                p->alive  = false;
                mi->alive = false;
                m_score += static_cast<int>(mi->scoreValue() * m_scoreMult);
                m_iceKilled++;
                m_fragments.emit(mi->pos, 180, 220, 255, 8, 10.f, 200.f);
            }
        }
    }

    // DataPacket update — drift, slow down, despawn; collect if avatar overlaps
    if (m_config.objective == NodeObjective::Extract) {
        for (auto& dp : m_dataPackets) {
            if (!dp.alive) continue;
            dp.pos     += dp.vel * dt;
            dp.vel     *= std::pow(0.1f, dt); // strong friction: halves ~every 0.3s
            dp.lifetime -= dt;
            if (dp.lifetime <= 0.f) { dp.alive = false; continue; }
            // Avatar collection
            if (m_avatar && m_avatar->alive) {
                if ((dp.pos - m_avatar->pos).length() < m_avatar->radius + dp.radius + 8.f) {
                    dp.alive = false;
                    m_packetsCollected++;
                    m_score += static_cast<int>(3.f * m_scoreMult);
                    m_particles.emit(dp.pos, 80, 220, 180, 6);
                }
            }
        }
    }

    // Boss update + projectile collision
    if (m_boss && m_boss->alive) {
        Vec2 playerPos = m_avatar ? m_avatar->pos : Vec2{Constants::SCREEN_WF*0.5f, Constants::SCREEN_HF*0.5f};
        auto bResult = m_boss->update(dt, playerPos);
        for (auto& ep : bResult.fired) m_enemyProjectiles.push_back(std::move(ep));
        if (bResult.phaseTransitioned) {
            m_bossPhaseTimer = 2.8f;
            m_shakeTimer     = 0.35f;
            m_shakeDuration  = 0.35f;
        }

        // Check player projectiles vs boss
        for (auto& p : m_projectiles) {
            if (!p->alive) continue;

            // Try shield hit first (ARCHON)
            if (m_boss->tryHitShield(p->pos, p->radius)) {
                p->alive = false;
                m_fragments.emit(p->pos, 200, 230, 255, 5, 8.f, 180.f);
                continue;
            }

            // Core hit
            if (m_boss->coreExposed() &&
                (p->pos - m_boss->pos).length() < m_boss->radius + p->radius) {
                p->alive = false;
                m_boss->hp--;
                m_fragments.emit(p->pos, 255, 120, 40, 4, 10.f, 200.f);

                if (m_boss->hp <= 0) {
                    m_boss->alive = false;
                    m_score += static_cast<int>(100.f * m_scoreMult);
                    m_iceKilled++;
                    // Big death explosion
                    m_fragments.emit(m_boss->pos, 255, 180, 40, 50, 22.f, 450.f);
                    m_fragments.emit(m_boss->pos, 255, 60,  20, 30, 16.f, 300.f);
                    m_fragments.emit(m_boss->pos, 255, 255, 200, 20, 12.f, 200.f);
                    // Start slow-mo + shake
                    m_bossDeathTimer = 2.5f;
                    m_shakeTimer     = 2.0f;
                    m_shakeDuration  = 2.0f;
                }
                break;
            }
        }
    }

    checkObjective(ctx);
}

// ---------------------------------------------------------------------------
// Mine update
// ---------------------------------------------------------------------------

void CombatScene::updateMines(float dt, SceneContext& ctx) {
    for (auto& mine : m_mines) {
        if (!mine->alive) continue;
        mine->update(dt);

        // Pulse damage: hits avatar and ICE in range
        if (mine->state() == PulseState::Pulse) {
            const float R = PulseMine::DAMAGE_RADIUS;

            // Damage avatar
            if (m_avatar && m_avatar->alive &&
                (m_avatar->pos - mine->pos).length() < R) {
                if (!m_avatar->shielded && !ctx.debugInvincible) {
                    if (m_avatar->extraLives > 0) {
                        m_avatar->extraLives--;
                        m_avatar->shieldTimer = 0.8f;
                        m_avatar->shielded    = true;
                        m_trace.onHit();
                    } else {
                        m_avatar->alive = false;
                        m_trace.onHit();
                        m_deathPos = m_avatar->pos;
                        m_deathTimer = 0.65f;
                        m_shakeTimer = m_shakeDuration = 0.5f;
                        m_fragments.emit(m_deathPos, Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 18, 16.f, 380.f);
                        m_particles.emit(m_deathPos,  Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 24);
                        return;
                    }
                }
            }

            // Also damages ICE (environmental chaos!)
            auto pulseDmg = [&](auto& vec) {
                for (auto& e : vec)
                    if (e->alive && (e->pos - mine->pos).length() < R) {
                        e->alive = false;
                        m_score += static_cast<int>(e->scoreValue() * m_scoreMult);
                        m_iceKilled++;
                    }
            };
            pulseDmg(m_hunters); pulseDmg(m_sentries); pulseDmg(m_spawnerICE); pulseDmg(m_phantoms); pulseDmg(m_leeches); pulseDmg(m_mirrors);
        }
    }
}

// ---------------------------------------------------------------------------
// Ricochet
// ---------------------------------------------------------------------------

void CombatScene::handleRicochet(bool hasRicochetMod) {
    if (!hasRicochetMod) return;
    constexpr float W = Constants::SCREEN_WF;
    constexpr float H = Constants::SCREEN_HF;
    for (auto& p : m_projectiles) {
        if (!p->alive || !p->noWrap) continue;
        if (p->bounced) { p->noWrap = false; continue; }
        bool hit = false;
        if (p->pos.x < 0.f)  { p->vel.x =  std::abs(p->vel.x); p->pos.x = 0.f;  hit = true; }
        if (p->pos.x > W)    { p->vel.x = -std::abs(p->vel.x); p->pos.x = W;     hit = true; }
        if (p->pos.y < 0.f)  { p->vel.y =  std::abs(p->vel.y); p->pos.y = 0.f;   hit = true; }
        if (p->pos.y > H)    { p->vel.y = -std::abs(p->vel.y); p->pos.y = H;      hit = true; }
        if (hit) { p->bounced = true; p->noWrap = false; }
    }
}

// ---------------------------------------------------------------------------
// Program activation
// ---------------------------------------------------------------------------

void CombatScene::activateProgram(int slot, SceneContext& ctx) {
    if (!ctx.programs || !ctx.programs->tryActivate(slot)) return;
    ProgramID pid = ctx.programs->slotID(slot);

    switch (pid) {
    case ProgramID::FRAG: {
        if (!m_avatar) break;
        const float offsets[] = { -0.349f, 0.f, 0.349f };
        for (float off : offsets) {
            float a      = m_avatar->angle + off;
            Vec2 heading = Vec2::fromAngle(a);
            Vec2 pos     = m_avatar->pos + heading * 18.f;
            m_projectiles.emplace_back(std::make_unique<Projectile>(pos, heading * Constants::PROJ_SPEED, a));
        }
        break;
    }
    case ProgramID::EMP:     m_empTimer     = 2.0f; break;
    case ProgramID::STEALTH: m_stealthTimer = 8.0f; break;
    case ProgramID::SHIELD:
        if (m_avatar) { m_avatar->shieldTimer = 2.0f; m_avatar->shielded = true; }
        break;
    case ProgramID::OVERDRIVE:
        if (m_avatar) m_avatar->overdriveTimer = 5.0f;
        break;
    case ProgramID::DECRYPT: {
        if (!m_avatar) break;
        Entity* nearest = nullptr; float best = std::numeric_limits<float>::max();
        auto check = [&](Entity* e) {
            if (!e->alive) return;
            float d = (e->pos - m_avatar->pos).length();
            if (d < best) { best = d; nearest = e; }
        };
        for (auto& h  : m_hunters)    check(h.get());
        for (auto& s  : m_sentries)   check(s.get());
        for (auto& sp : m_spawnerICE) check(sp.get());
        if (nearest) { nearest->alive = false; m_score += static_cast<int>(nearest->scoreValue() * m_scoreMult); m_iceKilled++; }
        break;
    }
    case ProgramID::FEEDBACK: {
        if (!m_avatar) break;
        for (auto& e : m_hunters)    if (e->alive && (e->pos-m_avatar->pos).length()<=180.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
        for (auto& e : m_sentries)   if (e->alive && (e->pos-m_avatar->pos).length()<=180.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
        for (auto& e : m_spawnerICE) if (e->alive && (e->pos-m_avatar->pos).length()<=180.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
        for (auto& e : m_phantoms)   if (e->alive && (e->pos-m_avatar->pos).length()<=180.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
        for (auto& e : m_leeches)    if (e->alive && (e->pos-m_avatar->pos).length()<=180.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
        for (auto& e : m_mirrors)    if (e->alive && (e->pos-m_avatar->pos).length()<=180.f) { e->alive=false; m_score += static_cast<int>(e->scoreValue() * m_scoreMult); m_iceKilled++; }
        break;
    }
    case ProgramID::CLONE: {
        if (!m_avatar) break;
        // Place decoy 200px in a random direction from the avatar
        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> aDist(0.f, 6.28318f);
        float a = aDist(rng);
        m_decoyPos   = m_avatar->pos + Vec2::fromAngle(a) * 200.f;
        m_decoyTimer = 4.0f;
        break;
    }
    case ProgramID::OVERCLOCK:
        m_overclockTimer = 6.0f;
        if (ctx.programs) ctx.programs->setCooldownMultiplier(m_baseCdMult * 0.5f);
        break;
    case ProgramID::BLACKOUT:
        m_enemyProjectiles.clear();
        break;
    case ProgramID::BREACH: {
        if (!m_avatar) break;
        // Fire a fast, long-lived, piercing projectile in the avatar's facing direction
        float a      = m_avatar->angle;
        Vec2  dir    = Vec2::fromAngle(a);
        Vec2  bpos   = m_avatar->pos + dir * 20.f;
        auto  beam   = std::make_unique<Projectile>(bpos, dir * (Constants::PROJ_SPEED * 3.f), a);
        beam->lifetime = 2.0f;
        beam->pierce   = true;
        m_projectiles.push_back(std::move(beam));
        break;
    }
    default: break;
    }
}

// ---------------------------------------------------------------------------
// Objective
// ---------------------------------------------------------------------------

void CombatScene::checkObjective(SceneContext& ctx) {
    if (m_complete) return;

    // Endless: trigger upgrade every 10 kills
    if (m_config.endless) {
        if (m_iceKilled > 0 && m_iceKilled >= m_lastUpgradeKills + 10) {
            m_lastUpgradeKills = m_iceKilled;
            m_complete = true;
            ctx.endlessWave++;
            ctx.scenes->replace(std::make_unique<NodeCompleteScene>(
                -1, m_score, m_iceKilled, 10, true, ctx.endlessWave, m_trace.trace()));
        }
        return;
    }

    bool done = false;
    switch (m_config.objective) {
    case NodeObjective::Sweep:   done = (m_iceKilled >= m_config.sweepTarget); break;
    case NodeObjective::Boss:    return; // boss completion handled via m_bossDeathTimer
    case NodeObjective::Survive: done = (m_surviveTimer <= 0.f); break;
    case NodeObjective::Extract: done = (m_packetsCollected >= m_config.extractTarget); break;
    }
    if (done) {
        m_complete = true;
        ctx.runNodes++;
        ctx.scenes->replace(std::make_unique<NodeCompleteScene>(
            m_config.nodeId, m_score, m_iceKilled, m_config.sweepTarget, false, 0));
    }
}

// ---------------------------------------------------------------------------
// Collisions / sweep
// ---------------------------------------------------------------------------

void CombatScene::handleCollisions() {
    m_collision.update(m_avatar.get(), m_projectiles,
                       m_hunters, m_sentries, m_spawnerICE, m_phantoms, m_leeches, m_mirrors, m_enemyProjectiles);
}

void CombatScene::sweepDead() {
    auto dead = [](const auto& e) { return !e->alive; };
    m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(), dead), m_projectiles.end());
    m_hunters.erase(std::remove_if(m_hunters.begin(), m_hunters.end(), dead), m_hunters.end());
    m_sentries.erase(std::remove_if(m_sentries.begin(), m_sentries.end(), dead), m_sentries.end());
    m_spawnerICE.erase(std::remove_if(m_spawnerICE.begin(), m_spawnerICE.end(), dead), m_spawnerICE.end());
    m_phantoms.erase(std::remove_if(m_phantoms.begin(), m_phantoms.end(), dead), m_phantoms.end());
    m_leeches.erase(std::remove_if(m_leeches.begin(), m_leeches.end(), dead), m_leeches.end());
    m_mirrors.erase(std::remove_if(m_mirrors.begin(), m_mirrors.end(), dead), m_mirrors.end());
    m_enemyProjectiles.erase(std::remove_if(m_enemyProjectiles.begin(), m_enemyProjectiles.end(), dead), m_enemyProjectiles.end());
    m_dataPackets.erase(std::remove_if(m_dataPackets.begin(), m_dataPackets.end(),
                        [](const DataPacket& p){ return !p.alive; }), m_dataPackets.end());
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void CombatScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;

    // Screen shake — apply viewport offset proportional to remaining shake intensity
    if (m_shakeTimer > 0.f && m_shakeDuration > 0.f) {
        float mag = 14.f * (m_shakeTimer / m_shakeDuration);
        static thread_local std::mt19937 srng(12345u);
        std::uniform_int_distribution<int> sd(-(int)mag, (int)mag);
        SDL_Rect vp = { sd(srng), sd(srng), Constants::SCREEN_W, Constants::SCREEN_H };
        SDL_RenderSetViewport(ctx.renderer, &vp);
    }

    vr->clear();

    const WorldTheme& wt = worldDef(ctx.currentWorld).theme;
    // Grid gets the world base color, node tier adds a subtle brightness shift
    float tierBoost = (m_config.tier - 1) * 0.15f;
    uint8_t gr = (uint8_t)(wt.gridR + wt.gridR * tierBoost);
    uint8_t gg = (uint8_t)(wt.gridG + wt.gridG * tierBoost);
    uint8_t gb = (uint8_t)(wt.gridB + wt.gridB * tierBoost);
    vr->drawGrid(gr, gg, gb);

    // Walls — dim glow in world accent color
    m_walls.render(vr, wt.accentR / 4, wt.accentG / 4, wt.accentB / 4);

    GlowColor projColor   = { Constants::COL_PROJ_R,   Constants::COL_PROJ_G,   Constants::COL_PROJ_B   };
    GlowColor avatarColor = { Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B };

    // Shield ring
    if (m_avatar && m_avatar->shielded) {
        GlowColor sc = { 80, 180, 255 };
        float r = m_avatar->radius + 8.f;
        constexpr int SEG = 20;
        for (int i = 0; i < SEG; ++i) {
            float a0 = i       * 6.28318f / SEG;
            float a1 = (i + 1) * 6.28318f / SEG;
            Vec2 p0 = m_avatar->pos + Vec2{ std::cos(a0) * r, std::sin(a0) * r };
            Vec2 p1 = m_avatar->pos + Vec2{ std::cos(a1) * r, std::sin(a1) * r };
            vr->drawGlowLine(p0, p1, sc);
        }
    }

    for (auto& p : m_projectiles) {
        const auto& v = p->worldVerts();
        if (v.size() >= 2) vr->drawGlowLine(v[0], v[1], projColor);
    }

    renderICE(ctx);
    renderMines(ctx);

    // DataPacket render — small pulsing diamonds in cyan-green
    if (m_config.objective == NodeObjective::Extract && ctx.vrenderer) {
        VectorRenderer* vr = ctx.vrenderer;
        for (const auto& dp : m_dataPackets) {
            if (!dp.alive) continue;
            // Draw diamond (4-vertex rotated square)
            float r = dp.radius;
            Vec2 verts[4] = {
                { dp.pos.x,     dp.pos.y - r },
                { dp.pos.x + r, dp.pos.y     },
                { dp.pos.x,     dp.pos.y + r },
                { dp.pos.x - r, dp.pos.y     },
            };
            // Pulse alpha based on lifetime
            float pulse = std::abs(std::sin(dp.lifetime * 4.f));
            uint8_t bright = static_cast<uint8_t>(160 + 95 * pulse);
            GlowColor dataCol = { 40, bright, static_cast<uint8_t>(bright / 2) };
            std::vector<Vec2> poly(std::begin(verts), std::end(verts));
            vr->drawGlowPoly(poly, dataCol);
            // Inner cross
            vr->drawGlowLine({ dp.pos.x - r * 0.5f, dp.pos.y },
                             { dp.pos.x + r * 0.5f, dp.pos.y }, dataCol);
            vr->drawGlowLine({ dp.pos.x, dp.pos.y - r * 0.5f },
                             { dp.pos.x, dp.pos.y + r * 0.5f }, dataCol);
        }
    }

    // Boss render
    if (m_boss && m_boss->alive) {
        static float s_time = 0.f;
        s_time += 0.016f; // approximate; fine for animation timing
        m_boss->render(vr, s_time);
    }

    if (m_avatar && m_avatar->alive)
        vr->drawGlowPoly(m_avatar->worldVerts(), avatarColor);

    // CLONE decoy — ghost ship at decoy position with pulsing dim color
    if (m_decoyTimer > 0.f && m_avatar) {
        float fade = std::min(m_decoyTimer / 4.f, 1.f);
        float dpulse = 0.4f + 0.3f * std::sin(SDL_GetTicks() * 0.006f);
        GlowColor dc = { (uint8_t)(60 * fade * dpulse),
                          (uint8_t)(200 * fade * dpulse),
                          (uint8_t)(255 * fade * dpulse) };
        auto verts = m_avatar->worldVerts();
        Vec2 off = m_decoyPos - m_avatar->pos;
        for (auto& v : verts) { v.x += off.x; v.y += off.y; }
        vr->drawGlowPoly(verts, dc);
    }

    if (m_empTimer > 0.f) {
        SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ctx.renderer, 0, 200, 200, 18);
        SDL_RenderFillRect(ctx.renderer, nullptr);
    }

    m_particles.render(ctx.renderer);
    m_fragments.render(ctx.renderer);

    vr->drawCRTOverlay();

    // Threshold crossing flash — full-screen color pulse
    if (m_thresholdFlash > 0.f) {
        float t = std::max(0.f, m_thresholdFlash);
        uint8_t fr = 255, fg = 60, fb = 20; // default orange-red
        if (m_thresholdFlashPct >= 100) { fr=255; fg=0;  fb=0;   } // red
        else if (m_thresholdFlashPct >= 75)  { fr=220; fg=0;  fb=255; } // magenta
        else if (m_thresholdFlashPct >= 50)  { fr=255; fg=120;fb=0;   } // orange
        else                                 { fr=255; fg=220;fb=0;   } // yellow
        SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ctx.renderer, fr, fg, fb, (uint8_t)(55 * t));
        SDL_RenderFillRect(ctx.renderer, nullptr);
        // Bright border ring
        SDL_SetRenderDrawColor(ctx.renderer, fr, fg, fb, (uint8_t)(120 * t));
        SDL_Rect border = { 0, 0, Constants::SCREEN_W, Constants::SCREEN_H };
        for (int i = 0; i < 4; ++i) {
            SDL_RenderDrawRect(ctx.renderer, &border);
            border = { border.x+1, border.y+1, border.w-2, border.h-2 };
        }
    }

    float glitchIntensity = std::max(0.f, (m_trace.trace() - 75.f) / 25.f);
    if (glitchIntensity > 0.f) vr->drawGlitch(glitchIntensity, SDL_GetTicks());

    bool hasController = (ctx.controller != nullptr);
    if (ctx.hud) ctx.hud->render(m_score, m_trace.trace(), ctx.programs, ctx.mods, hasController);

    if (ctx.hud) {
        std::string objStr;
        if (m_config.endless) {
            objStr = "WAVE " + std::to_string(ctx.endlessWave)
                   + "  ICE: " + std::to_string(m_iceKilled % 10) + "/10";
        } else if (m_config.objective == NodeObjective::Survive) {
            int s = std::max(0, (int)m_surviveTimer + 1);
            objStr = "SURVIVE: " + std::to_string(s) + "s";
        } else if (m_config.objective == NodeObjective::Extract) {
            objStr = "DATA: " + std::to_string(m_packetsCollected)
                   + " / " + std::to_string(m_config.extractTarget);
        } else {
            objStr = "ICE: " + std::to_string(m_iceKilled)
                   + " / " + std::to_string(m_config.sweepTarget);
        }
        SDL_Color cyan = { 100, 220, 200, 220 };
        int objW = ctx.hud->measureText(objStr);
        ctx.hud->drawLabel(objStr, Constants::SCREEN_W / 2 - objW / 2,
                           Constants::SCREEN_H - 38, cyan);
    }

    // Boss health bar
    if (m_boss && m_boss->alive && ctx.hud)
        ctx.hud->drawBossBar(m_boss->bossName(), m_boss->hp, m_boss->maxHp);

    // Boss phase 2 transition banner
    if (m_bossPhaseTimer > 0.f && ctx.hud) {
        float t = std::min(1.f, m_bossPhaseTimer / 2.8f);  // 0→1 as timer starts
        float fade = (t > 0.5f) ? 1.f : (t / 0.5f);        // fade in quick, hold, then fade
        if (m_bossPhaseTimer < 0.6f) fade = m_bossPhaseTimer / 0.6f;  // fade out last 0.6s
        uint8_t a = (uint8_t)(255 * fade);
        // Red flash border
        SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ctx.renderer, 255, 30, 30, (uint8_t)(40 * fade));
        SDL_RenderFillRect(ctx.renderer, nullptr);
        SDL_SetRenderDrawColor(ctx.renderer, 255, 30, 30, (uint8_t)(180 * fade));
        SDL_Rect b = {0, 0, Constants::SCREEN_W, Constants::SCREEN_H};
        for (int i = 0; i < 3; ++i) { SDL_RenderDrawRect(ctx.renderer, &b); b={b.x+1,b.y+1,b.w-2,b.h-2}; }
        // Text banner
        SDL_Color warn  = {255, 60,  60,  a};
        SDL_Color sub   = {220, 180, 180, a};
        std::string line1 = "-- PHASE 2 --";
        std::string line2 = "SYSTEM OVERRIDE";
        int cx = Constants::SCREEN_W / 2;
        int cy = Constants::SCREEN_H / 2 - 16;
        ctx.hud->drawLabel(line1, cx - ctx.hud->measureText(line1)/2, cy,      warn);
        ctx.hud->drawLabel(line2, cx - ctx.hud->measureText(line2)/2, cy + 22, sub);
    }

    // Reset viewport after screen shake
    if (m_shakeTimer > 0.f)
        SDL_RenderSetViewport(ctx.renderer, nullptr);

    if (m_paused)
        renderPauseOverlay(ctx);

    SDL_RenderPresent(ctx.renderer);
}

void CombatScene::renderICE(SceneContext& ctx) const {
    VectorRenderer* vr = ctx.vrenderer;
    GlowColor hunterColor  = { Constants::COL_HUNTER_R,  Constants::COL_HUNTER_G,  Constants::COL_HUNTER_B  };
    GlowColor sentryColor  = { Constants::COL_SENTRY_R,  Constants::COL_SENTRY_G,  Constants::COL_SENTRY_B  };
    GlowColor spawnerColor = { Constants::COL_SPAWNER_R, Constants::COL_SPAWNER_G, Constants::COL_SPAWNER_B };
    GlowColor eprojColor   = { Constants::COL_EPROJ_R,   Constants::COL_EPROJ_G,   Constants::COL_EPROJ_B   };

    for (auto& h  : m_hunters)  vr->drawGlowPoly(h->worldVerts(), hunterColor);
    for (auto& s  : m_sentries) {
        vr->drawGlowPoly(s->worldVerts(), sentryColor);
        // Outer threat ring — fixed circle showing danger radius
        GlowColor ringCol = { 100, 60, 0 };
        constexpr int RSEGS = 16;
        constexpr float RRAD = 26.f;
        for (int i = 0; i < RSEGS; ++i) {
            float a0 = i       * 6.28318f / RSEGS;
            float a1 = (i + 1) * 6.28318f / RSEGS;
            vr->drawGlowLine(
                s->pos + Vec2{ std::cos(a0) * RRAD, std::sin(a0) * RRAD },
                s->pos + Vec2{ std::cos(a1) * RRAD, std::sin(a1) * RRAD },
                ringCol);
        }
    }
    for (auto& sp : m_spawnerICE) {
        vr->drawGlowPoly(sp->worldVerts(), spawnerColor);
        const auto& v = sp->worldVerts();
        std::vector<Vec2> inner;
        inner.reserve(v.size());
        for (const auto& pt : v)
            inner.push_back(sp->pos + (pt - sp->pos) * 0.5f);
        vr->drawGlowPoly(inner, spawnerColor);
    }
    // Phantom ICE — draw at alpha proportional to visibility
    // During blink: strobe between full and dim to signal imminent fire.
    for (auto& ph : m_phantoms) {
        if (!ph->alive) continue;
        float vis = ph->visibility;
        if (ph->blinkTimer > 0.f) {
            // Strobe at ~8Hz — alternates bright / very dim
            float blink = std::sin(ph->blinkTimer * 50.27f); // 8 Hz
            vis = blink > 0.f ? 1.f : 0.15f;
        }
        if (vis < 0.02f) continue;  // fully cloaked — skip draw
        GlowColor phantomCol = { (uint8_t)(50 * vis), (uint8_t)(200 * vis), (uint8_t)(220 * vis) };
        vr->drawGlowPoly(ph->worldVerts(), phantomCol);
    }

    // Leech ICE — purple-magenta; pulses brighter when attached
    for (auto& lc : m_leeches) {
        if (!lc->alive) continue;
        float t = lc->attached ? (0.6f + 0.4f * std::sin(SDL_GetTicks() * 0.008f)) : 1.f;
        GlowColor leechCol = { (uint8_t)(180 * t), (uint8_t)(60 * t), (uint8_t)(255 * t) };
        vr->drawGlowPoly(lc->worldVerts(), leechCol);
    }

    // Mirror ICE — silver/white, drawn as an octagon with a bright reflective face line
    for (auto& mi : m_mirrors) {
        if (!mi->alive) continue;
        GlowColor mirrorCol = { 200, 230, 255 };
        vr->drawGlowPoly(mi->worldVerts(), mirrorCol);
        // Draw the reflecting face: a bright line perpendicular to the facing direction
        // The face is a chord on the front side of the circle
        Vec2 perp = { -std::sin(mi->facingAngle), std::cos(mi->facingAngle) };
        Vec2 faceA = mi->pos + perp * (mi->radius * 0.85f);
        Vec2 faceB = mi->pos - perp * (mi->radius * 0.85f);
        GlowColor faceCol = { 255, 255, 255 };
        vr->drawGlowLine(faceA, faceB, faceCol);
    }

    for (auto& ep : m_enemyProjectiles) {
        const auto& v = ep->worldVerts();
        if (v.size() >= 2) vr->drawGlowLine(v[0], v[1], eprojColor);
    }
}

// ---------------------------------------------------------------------------
// Mine rendering
// ---------------------------------------------------------------------------

void CombatScene::renderMines(SceneContext& ctx) const {
    SDL_Renderer* r = ctx.renderer;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    for (const auto& mine : m_mines) {
        if (!mine->alive) continue;

        // Hexagon body — hazard yellow-green (distinct from sentry orange)
        GlowColor mineCol = { 160, 255, 0 };
        ctx.vrenderer->drawGlowPoly(mine->worldVerts(), mineCol);
        // Inner crosshair
        GlowColor crossCol = { 80, 200, 0 };
        float mcx = mine->pos.x, mcy = mine->pos.y;
        ctx.vrenderer->drawGlowLine({mcx - 7.f, mcy}, {mcx + 7.f, mcy}, crossCol);
        ctx.vrenderer->drawGlowLine({mcx, mcy - 7.f}, {mcx, mcy + 7.f}, crossCol);

        // HP indicator dots
        for (int i = 0; i < mine->hitsLeft; ++i) {
            SDL_SetRenderDrawColor(r, 160, 255, 40, 200);
            SDL_Rect dot = { (int)mine->pos.x - 8 + i * 8, (int)mine->pos.y + 14, 4, 4 };
            SDL_RenderFillRect(r, &dot);
        }

        // Warning ring (expands during warning phase)
        float warnFrac = mine->warningFrac();
        if (warnFrac > 0.f) {
            float ringR   = mine->pulseRadius();
            int   segs    = 24;
            uint8_t alpha = (uint8_t)(200 * warnFrac);
            SDL_SetRenderDrawColor(r, 255, 80, 0, alpha);
            for (int i = 0; i < segs; ++i) {
                float a0 = i       * 6.28318f / segs;
                float a1 = (i + 1) * 6.28318f / segs;
                SDL_RenderDrawLine(r,
                    (int)(mine->pos.x + std::cos(a0) * ringR),
                    (int)(mine->pos.y + std::sin(a0) * ringR),
                    (int)(mine->pos.x + std::cos(a1) * ringR),
                    (int)(mine->pos.y + std::sin(a1) * ringR));
            }
        }

        // Full pulse flash
        if (mine->state() == PulseState::Pulse) {
            float R = PulseMine::DAMAGE_RADIUS;
            SDL_SetRenderDrawColor(r, 255, 100, 0, 50);
            SDL_Rect flash = {
                (int)(mine->pos.x - R), (int)(mine->pos.y - R),
                (int)(R * 2),           (int)(R * 2)
            };
            SDL_RenderFillRect(r, &flash);
        }
    }
}

// ---------------------------------------------------------------------------
// Death effects
// ---------------------------------------------------------------------------

void CombatScene::emitDeathParticles() {
    auto emit = [this](const auto& vec, uint8_t r, uint8_t g, uint8_t b) {
        for (const auto& e : vec)
            if (!e->alive) m_particles.emit(e->pos, r, g, b, 8);
    };
    emit(m_hunters,          Constants::COL_HUNTER_R,  Constants::COL_HUNTER_G,  Constants::COL_HUNTER_B);
    emit(m_sentries,         Constants::COL_SENTRY_R,  Constants::COL_SENTRY_G,  Constants::COL_SENTRY_B);
    emit(m_spawnerICE,       Constants::COL_SPAWNER_R, Constants::COL_SPAWNER_G, Constants::COL_SPAWNER_B);
    emit(m_phantoms,         50, 200, 220);
    emit(m_leeches,          180, 60, 255);
    emit(m_mirrors,          200, 230, 255);
    emit(m_enemyProjectiles, Constants::COL_EPROJ_R,   Constants::COL_EPROJ_G,   Constants::COL_EPROJ_B);
}

void CombatScene::emitDeathFragments() {
    for (const auto& e : m_hunters)
        if (!e->alive)
            m_fragments.emit(e->pos, Constants::COL_HUNTER_R, Constants::COL_HUNTER_G, Constants::COL_HUNTER_B, 6, 10.f, 320.f);
    for (const auto& e : m_sentries)
        if (!e->alive)
            m_fragments.emit(e->pos, Constants::COL_SENTRY_R, Constants::COL_SENTRY_G, Constants::COL_SENTRY_B, 8, 14.f, 240.f);
    for (const auto& e : m_spawnerICE)
        if (!e->alive)
            m_fragments.emit(e->pos, Constants::COL_SPAWNER_R, Constants::COL_SPAWNER_G, Constants::COL_SPAWNER_B, 12, 18.f, 200.f);
    for (const auto& e : m_phantoms)
        if (!e->alive)
            m_fragments.emit(e->pos, 50, 200, 220, 8, 12.f, 300.f);
    for (const auto& e : m_leeches)
        if (!e->alive)
            m_fragments.emit(e->pos, 180, 60, 255, 6, 10.f, 280.f);
    for (const auto& e : m_mirrors)
        if (!e->alive)
            m_fragments.emit(e->pos, 200, 230, 255, 8, 12.f, 250.f);
}

// ---------------------------------------------------------------------------
// Pause overlay
// ---------------------------------------------------------------------------

static std::string pauseVolBar(int vol) {
    int filled = vol / 10;
    std::string bar = "[";
    for (int i = 0; i < 10; ++i) bar += (i < filled ? "=" : " ");
    bar += "] " + std::to_string(vol) + "%";
    return bar;
}

void CombatScene::pauseChangeVolume(int item, int delta, SceneContext& ctx) {
    if (item == 1) {
        m_pauseMusicVol = std::max(0, std::min(100, m_pauseMusicVol + delta * 10));
        if (ctx.audio) ctx.audio->setMusicVolume(m_pauseMusicVol * MIX_MAX_VOLUME / 100);
    } else if (item == 2) {
        m_pauseSfxVol = std::max(0, std::min(100, m_pauseSfxVol + delta * 10));
        if (ctx.audio) ctx.audio->setSfxVolume(m_pauseSfxVol * MIX_MAX_VOLUME / 100);
    }
}

void CombatScene::renderPauseOverlay(SceneContext& ctx) const {
    SDL_Renderer* r = ctx.renderer;
    if (!r || !ctx.hud) return;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Dark translucent backdrop
    SDL_SetRenderDrawColor(r, 0, 0, 0, 170);
    SDL_RenderFillRect(r, nullptr);

    float pulse = 0.5f + 0.5f * std::sin(m_pauseTime * 3.f);

    // Two-panel layout: left = menu, right = build chart
    constexpr int MENU_W  = 340;
    constexpr int CHART_W = 280;
    constexpr int GAP     = 16;
    constexpr int PANEL_H = 320;
    constexpr int TOTAL_W = MENU_W + GAP + CHART_W;

    int panelX = Constants::SCREEN_W / 2 - TOTAL_W / 2;
    int panelY = Constants::SCREEN_H / 2 - PANEL_H / 2;
    int chartX = panelX + MENU_W + GAP;

    // Left menu panel
    SDL_SetRenderDrawColor(r, 0, 14, 10, 230);
    SDL_Rect menuBg = { panelX, panelY, MENU_W, PANEL_H };
    SDL_RenderFillRect(r, &menuBg);
    SDL_SetRenderDrawColor(r, 0, (Uint8)(180 + 60 * pulse), 120, 220);
    SDL_RenderDrawRect(r, &menuBg);

    SDL_Color titleCol = { 0, (Uint8)(210 + 45 * pulse), 150, 255 };
    ctx.hud->drawLabel("// PAUSED //", panelX + 16, panelY + 14, titleCol);
    SDL_SetRenderDrawColor(r, 0, 80, 60, 140);
    SDL_RenderDrawLine(r, panelX+12, panelY+44, panelX+MENU_W-12, panelY+44);

    struct Entry { const char* label; std::string value; };
    Entry entries[] = {
        { "RESUME",       "" },
        { "MUSIC VOLUME", pauseVolBar(m_pauseMusicVol) },
        { "SFX VOLUME",   pauseVolBar(m_pauseSfxVol)   },
        { "QUIT TO MENU", "" },
    };

    int ry = panelY + 54;
    for (int i = 0; i < 4; ++i) {
        bool sel = (m_pauseCursor == i);
        float p  = sel ? pulse : 0.f;
        SDL_Color labelCol = sel ? SDL_Color{0,(Uint8)(200+40*p),(Uint8)(160+60*p),255}
                                 : SDL_Color{70, 130, 100, 200};
        SDL_Color valCol   = sel ? SDL_Color{220, 220, 80, 255}
                                 : SDL_Color{150, 200, 80, 200};

        bool isVol = !entries[i].value.empty();
        int rowH = isVol ? 46 : 26;
        if (sel) {
            SDL_SetRenderDrawColor(r, 0, (Uint8)(30+15*p), (Uint8)(22+10*p), 120);
            SDL_Rect row = { panelX+8, ry-3, MENU_W-16, rowH };
            SDL_RenderFillRect(r, &row);
        }

        std::string lbl = sel ? ("> " + std::string(entries[i].label))
                               : ("  " + std::string(entries[i].label));
        ctx.hud->drawLabel(lbl.c_str(), panelX+16, ry, labelCol);
        if (isVol) {
            ctx.hud->drawLabel(entries[i].value.c_str(), panelX+26, ry+22, valCol);
            ry += 62;
        } else {
            ry += 44;
        }
    }

    SDL_Color hintCol = { 50, 90, 70, 160 };
    ctx.hud->drawLabel("ESC/START: resume  L/R: vol",
                       panelX+12, panelY+PANEL_H-22, hintCol);

    // Right build chart panel
    renderBuildChart(r, ctx.hud, ctx.mods, ctx.programs, ctx.saveData,
                     pulse, chartX, panelY, CHART_W, PANEL_H);
}
