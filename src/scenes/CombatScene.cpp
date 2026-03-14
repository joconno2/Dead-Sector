// CombatScene.cpp — Lifecycle, setup, input events, mine spawning
#include "CombatScene.hpp"
#include "core/Bindings.hpp"
#include "audio/AudioSystem.hpp"
#include "steam/SteamManager.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "RunSummaryScene.hpp"
#include "core/Constants.hpp"
#include "core/SaveSystem.hpp"
#include "core/Worlds.hpp"
#include "systems/ModSystem.hpp"

#include <SDL.h>
#include <cmath>
#include <random>

CombatScene::CombatScene(NodeConfig cfg) : m_config(cfg) {}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void CombatScene::onEnter(SceneContext& ctx) {
    // Cooldown mult: mod-based × shop COOL_EXEC stacks
    m_baseCdMult = ctx.mods ? ctx.mods->cdMult() : 1.f;
    if (ctx.saveData) {
        int stacks = ctx.saveData->purchaseCount("COOL_EXEC");
        for (int i = 0; i < stacks; ++i) m_baseCdMult *= 0.85f;
    }
    if (ctx.programs) ctx.programs->setCooldownMultiplier(m_baseCdMult);
    if (ctx.programs) ctx.programs->resetCooldowns();

    // Bonus lives from event nodes
    if (ctx.bonusLives > 0 && ctx.saveData) {
        ctx.saveData->purchases.push_back("_BONUS_LIFE_PLACEHOLDER");
    }

    m_audio      = ctx.audio;
    m_controller = ctx.controller;
    m_steam      = ctx.steam;

    // Combat music plays at 65% of user's preferred volume — the track is
    // intrinsically loud and would otherwise drown out menu music by comparison.
    if (m_audio) {
        int vol = ctx.saveData ? ctx.saveData->musicVolume : 80;
        m_audio->setMusicVolume(vol * SDL_MIX_MAXVOLUME / 100 * 65 / 100);
        m_audio->playMusic("assets/music/Karl Casey - Fortress.mp3");
    }
    m_pauseMusicVol = ctx.saveData ? ctx.saveData->musicVolume : 80;
    m_pauseSfxVol   = ctx.saveData ? ctx.saveData->sfxVolume   : 80;

    // Apply bonus lives from context (set by event nodes)
    resetGame(ctx);
    if (ctx.bonusLives > 0 && m_avatar) {
        m_avatar->extraLives += ctx.bonusLives;
        ctx.bonusLives = 0;
    }

    setupCollisionCallback(ctx);

    // Trace config
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
        if (m_controller) {
            Uint16 lo = (Uint16)(0x1500 * (pct / 25));
            SDL_GameControllerRumble(m_controller, lo, lo / 2, 150u + (Uint32)(pct / 25) * 50u);
        }
        if (pct >= 100 && !m_highTraceUnlocked) {
            m_highTraceUnlocked = true;
            if (m_steam) m_steam->unlockAchievement(ACH_HIGH_TRACE);
        }
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
            if (!m_firstKillUnlocked) {
                m_firstKillUnlocked = true;
                if (m_steam) m_steam->unlockAchievement(ACH_FIRST_KILL);
            }
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
                if (m_controller) SDL_GameControllerRumble(m_controller, 0x2000, 0x3000, 150);
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
            m_deathTimer = 1.8f;
            m_shakeTimer = m_shakeDuration = 0.5f;
            if (m_controller) SDL_GameControllerRumble(m_controller, 0x6000, 0xA000, 400);
            if (m_audio) m_audio->stopMusic();
            m_fragments.emit(m_deathPos, Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 28, 18.f, 460.f);
            m_particles.emit(m_deathPos,  Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 36);

        } else if (ev.type == CollisionEvent::Type::EnemyProjectileHitAvatar) {
            if (!ev.avatar->alive || !ev.enemyProj->alive) return;
            if (ev.avatar->shielded || *invincible) { ev.enemyProj->alive = false; return; }
            if (ev.avatar->extraLives > 0) {
                ev.enemyProj->alive = false;
                ev.avatar->extraLives--;
                ev.avatar->shieldTimer = 0.8f;
                ev.avatar->shielded    = true;
                m_trace.onHit();
                if (m_controller) SDL_GameControllerRumble(m_controller, 0x2000, 0x3000, 150);
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
            m_deathTimer = 1.8f;
            m_shakeTimer = m_shakeDuration = 0.5f;
            if (m_controller) SDL_GameControllerRumble(m_controller, 0x6000, 0xA000, 400);
            if (m_audio) m_audio->stopMusic();
            m_fragments.emit(m_deathPos, Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 28, 18.f, 460.f);
            m_particles.emit(m_deathPos,  Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 36);

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
                if (m_controller) SDL_GameControllerRumble(m_controller, 0x2000, 0x3000, 150);
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
            m_deathTimer = 1.8f;
            m_shakeTimer = m_shakeDuration = 0.5f;
            if (m_controller) SDL_GameControllerRumble(m_controller, 0x6000, 0xA000, 400);
            if (m_audio) m_audio->stopMusic();
            m_fragments.emit(m_deathPos, Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 28, 18.f, 460.f);
            m_particles.emit(m_deathPos,  Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 36);
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
    m_firstKillUnlocked = false;
    m_highTraceUnlocked = false;
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

    m_drones.clear();
    m_shockwaves.clear();
    m_beams.clear();
    m_particles.clear();
    m_fragments.clear();
    m_trace.reset();
    if (m_config.startTrace > 0.f) m_trace.add(m_config.startTrace);
    m_trace.setTickRate(m_config.traceTickRate);
    m_spawner.reset();

    // Hull selection from save
    HullType hull = HullType::Delta;
    if (ctx.saveData) hull = hullFromString(ctx.saveData->activeHull);
    m_isGolden = ctx.saveData && ctx.saveData->isGolden(ctx.saveData->activeHull);

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
    std::uniform_int_distribution<int> countDist(tier - 2, tier - 1);
    int count = countDist(rng);

    const float W = Constants::SCREEN_WF;
    const float H = Constants::SCREEN_HF;
    std::uniform_real_distribution<float> xDist(60.f, W - 60.f);
    std::uniform_real_distribution<float> yDist(60.f, H - 60.f);

    for (int i = 0; i < count; ++i) {
        for (int attempt = 0; attempt < 20; ++attempt) {
            float mx = xDist(rng), my = yDist(rng);
            float dx = mx - W * 0.5f, dy = my - H * 0.5f;
            if (std::sqrt(dx*dx + dy*dy) < 160.f) continue;
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
// Input / pause events
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
