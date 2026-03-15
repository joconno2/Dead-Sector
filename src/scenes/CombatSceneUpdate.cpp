// CombatSceneUpdate.cpp — update, AI, programs, objective, collision helpers
#include "CombatScene.hpp"
#include "entities/BossEnemy.hpp"
#include "audio/AudioSystem.hpp"
#include "steam/SteamManager.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "GameOverScene.hpp"
#include "NodeCompleteScene.hpp"
#include "VictoryScene.hpp"
#include "WorldCompleteScene.hpp"
#include "core/Constants.hpp"
#include "core/Worlds.hpp"
#include "systems/ModSystem.hpp"
#include "entities/Avatar.hpp"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

void CombatScene::update(float dt, SceneContext& ctx) {
    if (m_complete) return;
    if (m_paused) { m_pauseTime += dt; return; }
    const int killsAtFrameStart = m_iceKilled;

    // Avatar death — slow-mo like boss death, music already fading
    if (m_deathTimer > 0.f) {
        m_deathTimer -= dt;
        float slowDt = dt * 0.12f;
        m_particles.update(slowDt);
        m_fragments.update(slowDt);
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
        dt *= 0.10f;
        if (m_bossDeathTimer <= 0.f) {
            if (m_config.endless) {
                m_boss.reset();
                return;
            }
            m_complete = true;
            ctx.runNodes++;
            if (ctx.currentWorld < 2) {
                ctx.scenes->replace(std::make_unique<WorldCompleteScene>(
                    ctx.currentWorld, m_score, m_iceKilled));
            } else {
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
    float pRadMult    = (ctx.mods ? ctx.mods->projRadiusMult() : 1.f) * hs.projRadiusMult;
    bool  splitRound  = ctx.mods ? ctx.mods->hasSplitRound()  : false;
    bool  overcharge  = ctx.mods ? ctx.mods->hasOvercharge()   : false;
    bool  scatterCore = ctx.mods ? ctx.mods->hasScatterCore()  : false;

    if (m_avatar && m_avatar->alive) {
        if (m_input.rotLeft)       m_avatar->rotateLeft(dt  * rotMult);
        if (m_input.rotRight)      m_avatar->rotateRight(dt * rotMult);
        if (m_input.thrustForward) m_avatar->applyThrust(dt, thrustMult, speedCapMult);

        if (m_input.thrustForward) {
            Vec2 heading    = Vec2::fromAngle(m_avatar->angle);
            Vec2 exhaustDir = { -heading.x, -heading.y };
            Vec2 nozzle = m_avatar->pos - heading * 28.f;
            m_particles.emitExhaust(nozzle, exhaustDir, m_avatar->vel, 255, 255, 220, 2);
            m_particles.emitExhaust(nozzle, exhaustDir, m_avatar->vel,  80, 210, 255, 4);
            m_particles.emitExhaust(nozzle, exhaustDir, m_avatar->vel,  30, 100, 220, 2);
        }

        if (m_isGolden) {
            m_sparkleTimer += dt;
            if (m_sparkleTimer >= 0.07f) {
                m_sparkleTimer = 0.f;
                m_particles.emit(m_avatar->pos, 255, 210, 30, 2);
            }
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

                // TWIN_SHOT: fire a parallel twin projectile offset perpendicular
                // (active via program timer OR built-in hull trait on RAPTOR)
                if (m_twinShotTimer > 0.f || hs.builtInTwinShot) {
                    Vec2 h2   = Vec2::fromAngle(m_avatar->angle);
                    Vec2 perp = { -h2.y, h2.x };
                    Vec2 twinPos2 = m_avatar->pos + h2 * 18.f + perp * 14.f;
                    auto twin2 = std::make_unique<Projectile>(twinPos2, h2 * (Constants::PROJ_SPEED * pSpeedMult), m_avatar->angle);
                    twin2->radius *= pRadMult;
                    if (hasRicochetMod) twin2->noWrap = true;
                    m_projectiles.push_back(std::move(twin2));
                }

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
    if (m_decoyTimer    > 0.f) m_decoyTimer    -= dt;
    if (m_twinShotTimer > 0.f) m_twinShotTimer -= dt;

    // GRAVITY_WELL — pull all ICE toward screen center each frame
    if (m_gravTimer > 0.f) {
        m_gravTimer -= dt;
        Vec2 center = { Constants::SCREEN_WF * 0.5f, Constants::SCREEN_HF * 0.5f };
        const float PULL = 180.f;
        auto pull = [&](Entity* e) {
            if (!e->alive) return;
            Vec2 toCenter = center - e->pos;
            float dist = toCenter.length();
            if (dist > 1.f) e->vel = e->vel + toCenter * (PULL * dt / dist);
        };
        for (auto& h  : m_hunters)    pull(h.get());
        for (auto& s  : m_sentries)   pull(s.get());
        for (auto& sp : m_spawnerICE) pull(sp.get());
        for (auto& ph : m_phantoms)   pull(ph.get());
        for (auto& lc : m_leeches)    pull(lc.get());
        for (auto& mi : m_mirrors)    pull(mi.get());
    }

    // BEACON — auto-turret fires at nearest ICE every 0.5s
    if (m_beaconTimer > 0.f) {
        m_beaconTimer  -= dt;
        m_beaconFireCd -= dt;
        if (m_beaconFireCd <= 0.f) {
            m_beaconFireCd = 0.5f;
            Entity* target = nullptr; float bestDist = std::numeric_limits<float>::max();
            auto check = [&](Entity* e) {
                if (!e->alive) return;
                float d = (e->pos - m_beaconPos).length();
                if (d < bestDist) { bestDist = d; target = e; }
            };
            for (auto& h  : m_hunters)    check(h.get());
            for (auto& s  : m_sentries)   check(s.get());
            for (auto& sp : m_spawnerICE) check(sp.get());
            for (auto& ph : m_phantoms)   check(ph.get());
            for (auto& lc : m_leeches)    check(lc.get());
            for (auto& mi : m_mirrors)    check(mi.get());
            if (target) {
                Vec2 dir = (target->pos - m_beaconPos);
                float len = dir.length();
                if (len > 0.1f) {
                    dir = dir * (1.f / len);
                    float ang = std::atan2(dir.y, dir.x);
                    auto shot = std::make_unique<Projectile>(
                        m_beaconPos + dir * 14.f,
                        dir * Constants::PROJ_SPEED, ang);
                    m_projectiles.push_back(std::move(shot));
                }
            }
        }
    }

    // NOVA_RING — burst 8 shots radially every 1.5s
    if (m_novaTimer > 0.f) {
        m_novaTimer  -= dt;
        m_novaFireCd -= dt;
        if (m_novaFireCd <= 0.f && m_avatar && m_avatar->alive) {
            m_novaFireCd = 1.5f;
            constexpr int SPOKES = 8;
            for (int i = 0; i < SPOKES; ++i) {
                float a   = i * (6.28318f / SPOKES);
                Vec2  dir = Vec2::fromAngle(a);
                auto  p   = std::make_unique<Projectile>(
                    m_avatar->pos + dir * 16.f,
                    dir * Constants::PROJ_SPEED, a);
                m_projectiles.push_back(std::move(p));
            }
            if (m_audio) m_audio->playShot();
        }
    }

    // Drones — orbit avatar, fire at nearest ICE every 0.6s, expire after life runs out
    if (m_avatar && m_avatar->alive && !m_drones.empty()) {
        constexpr float ORBIT_R    = 50.f;
        constexpr float ORBIT_SPEED= 2.8f;
        constexpr float FIRE_CD    = 0.6f;
        for (auto& d : m_drones) {
            d.orbitAngle += ORBIT_SPEED * dt;
            d.life       -= dt;
            d.fireTimer  -= dt;
            if (d.fireTimer <= 0.f) {
                d.fireTimer = FIRE_CD;
                // find nearest ICE
                Vec2    dPos  = m_avatar->pos + Vec2{ std::cos(d.orbitAngle) * ORBIT_R,
                                                       std::sin(d.orbitAngle) * ORBIT_R };
                Entity* target = nullptr; float best = std::numeric_limits<float>::max();
                auto chk = [&](Entity* e){ if (!e->alive) return; float dd=(e->pos-dPos).length(); if(dd<best){best=dd;target=e;} };
                for (auto& h  : m_hunters)    chk(h.get());
                for (auto& s  : m_sentries)   chk(s.get());
                for (auto& sp : m_spawnerICE) chk(sp.get());
                for (auto& ph : m_phantoms)   chk(ph.get());
                for (auto& lc : m_leeches)    chk(lc.get());
                for (auto& mi : m_mirrors)    chk(mi.get());
                if (target) {
                    Vec2 dir = target->pos - dPos;
                    float len = dir.length();
                    if (len > 0.1f) {
                        dir = dir * (1.f / len);
                        float a = std::atan2(dir.y, dir.x);
                        m_projectiles.push_back(std::make_unique<Projectile>(dPos + dir * 10.f, dir * Constants::PROJ_SPEED, a));
                    }
                }
            }
        }
        m_drones.erase(std::remove_if(m_drones.begin(), m_drones.end(),
                        [](const Drone& d){ return d.life <= 0.f; }), m_drones.end());
    }

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
        if (m_avatar && m_avatar->alive)
            m_walls.resolveCircle(m_avatar->pos, m_avatar->vel, m_avatar->radius, 0.6f);

        for (auto& p : m_projectiles) {
            if (!p->alive) continue;
            bool wasNoWrap = p->noWrap;
            p->noWrap = true;
            if (m_walls.resolveCircle(p->pos, p->vel, p->radius, 0.9f)) {
                if (!wasNoWrap) p->noWrap = false;
            } else {
                p->noWrap = wasNoWrap;
            }
        }

        for (auto& ep : m_enemyProjectiles)
            if (ep->alive) m_walls.resolveCircle(ep->pos, ep->vel, ep->radius, 0.9f);

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
        if (aiResult.leechTraceGain > 0.f) m_trace.add(aiResult.leechTraceGain);
    }

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
                    m_fragments.emit(mine->pos, 255, 140, 20, 16, 16.f, 360.f);
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

    // ACH_CHAIN_KILL: 5 kills within a 10-second rolling window
    if (m_steam && !m_chainKillUnlocked) {
        int delta = m_iceKilled - killsAtFrameStart;
        if (delta > 0) {
            m_chainKillCount += delta;
            m_chainKillTimer  = 10.f;
            if (m_chainKillCount >= 5) {
                m_steam->unlockAchievement(ACH_CHAIN_KILL);
                m_steam->checkCompletionist();
                m_chainKillUnlocked = true;
            }
        } else {
            m_chainKillTimer -= dt;
            if (m_chainKillTimer <= 0.f) { m_chainKillTimer = 0.f; m_chainKillCount = 0; }
        }
    }

    // ACH_ENDLESS_10 / ACH_ENDLESS_25
    if (m_config.endless && m_steam) {
        if (ctx.endlessWave >= 10 && !m_endlessAch10Unlocked) {
            m_steam->unlockAchievement(ACH_ENDLESS_10);
            m_steam->checkCompletionist();
            m_endlessAch10Unlocked = true;
        }
        if (ctx.endlessWave >= 25 && !m_endlessAch25Unlocked) {
            m_steam->unlockAchievement(ACH_ENDLESS_25);
            m_steam->checkCompletionist();
            m_endlessAch25Unlocked = true;
        }
    }
    m_mines.erase(std::remove_if(m_mines.begin(), m_mines.end(),
                  [](const auto& m){ return !m->alive; }), m_mines.end());

    m_particles.update(dt);
    m_fragments.update(dt);

    if (m_stealthTimer <= 0.f) m_trace.update(dt);

    int liveCount = (int)(m_hunters.size() + m_sentries.size() + m_spawnerICE.size() + m_phantoms.size() + m_leeches.size() + m_mirrors.size());
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
            Vec2 faceDir = { std::cos(mi->facingAngle), std::sin(mi->facingAngle) };
            Vec2 toProj  = (p->pos - mi->pos).normalized();
            float dot    = faceDir.x * toProj.x + faceDir.y * toProj.y;
            if (dot > 0.f) {
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
                p->alive  = false;
                mi->alive = false;
                m_score += static_cast<int>(mi->scoreValue() * m_scoreMult);
                m_iceKilled++;
                m_fragments.emit(mi->pos, 180, 220, 255, 8, 10.f, 200.f);
            }
        }
    }

    // DataPacket update
    if (m_config.objective == NodeObjective::Extract) {
        for (auto& dp : m_dataPackets) {
            if (!dp.alive) continue;
            dp.pos     += dp.vel * dt;
            dp.vel     *= std::pow(0.1f, dt);
            dp.lifetime -= dt;
            if (dp.lifetime <= 0.f) { dp.alive = false; continue; }
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

        for (auto& p : m_projectiles) {
            if (!p->alive) continue;

            if (m_boss->tryHitShield(p->pos, p->radius)) {
                p->alive = false;
                m_fragments.emit(p->pos, 200, 230, 255, 5, 8.f, 180.f);
                continue;
            }

            if (m_boss->coreExposed() &&
                (p->pos - m_boss->pos).length() < m_boss->radius + p->radius) {
                p->alive = false;
                m_boss->hp--;
                m_fragments.emit(p->pos, 255, 120, 40, 4, 10.f, 200.f);

                if (m_boss->hp <= 0) {
                    m_boss->alive = false;
                    m_score += static_cast<int>(100.f * m_scoreMult);
                    m_iceKilled++;
                    if (m_steam) {
                        m_steam->unlockAchievement(ACH_FIRST_BOSS);
                        switch (m_boss->bossType) {
                        case BossType::Manticore: m_steam->unlockAchievement(ACH_KILL_MANTICORE); break;
                        case BossType::Archon:    m_steam->unlockAchievement(ACH_KILL_ARCHON);    break;
                        case BossType::Vortex:    m_steam->unlockAchievement(ACH_KILL_VORTEX);    break;
                        }
                        if (!m_speedrunUnlocked) {
                            Uint32 elapsed = SDL_GetTicks() - m_combatStartTicks;
                            if (elapsed <= 2 * 60 * 1000) {
                                m_steam->unlockAchievement(ACH_SPEEDRUN);
                                m_speedrunUnlocked = true;
                            }
                        }
                        m_steam->checkCompletionist();
                    }
                    m_fragments.emit(m_boss->pos, 255, 180, 40, 70, 24.f, 520.f);
                    m_fragments.emit(m_boss->pos, 255, 60,  20, 45, 18.f, 360.f);
                    m_fragments.emit(m_boss->pos, 255, 255, 200, 30, 14.f, 240.f);
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

        if (mine->state() == PulseState::Pulse) {
            const float R = PulseMine::DAMAGE_RADIUS;

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
                        m_deathTimer = 1.8f;
                        m_shakeTimer = m_shakeDuration = 0.5f;
                        if (ctx.audio) ctx.audio->stopMusic();
                        m_fragments.emit(m_deathPos, Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 18, 16.f, 380.f);
                        m_particles.emit(m_deathPos,  Constants::COL_AVATAR_R, Constants::COL_AVATAR_G, Constants::COL_AVATAR_B, 24);
                        return;
                    }
                }
            }

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

    // ACH_ALL_PROGRAMS: track each unique program type used this run
    if (pid != ProgramID::NONE) {
        ctx.programsUsedThisRun |= static_cast<uint16_t>(1u << static_cast<int>(pid));
        static constexpr uint16_t ALL_15 = (1u << 15) - 1; // ProgramID 0-14
        if ((ctx.programsUsedThisRun & ALL_15) == ALL_15 && m_steam && !m_allProgramsUnlocked) {
            m_steam->unlockAchievement(ACH_ALL_PROGRAMS);
            m_steam->checkCompletionist();
            m_allProgramsUnlocked = true;
        }
    }

    switch (pid) {
    case ProgramID::FRAG: {
        // Deploy 2 attack drones that orbit the avatar and auto-fire at nearest ICE
        m_drones.clear();  // replace any existing drones
        m_drones.push_back({ 0.f,             0.f, 8.0f });  // drone 1: starts at 0°
        m_drones.push_back({ 3.14159f,        0.f, 8.0f });  // drone 2: starts at 180°
        break;
    }
    case ProgramID::EMP:
        m_empTimer = 2.0f;
        // ACH_EMP_MULTI: hit 5+ ICE with a single EMP
        if (m_steam && !m_empMultiUnlocked) {
            int empHits = 0;
            for (auto& e : m_hunters)    if (e->alive) empHits++;
            for (auto& e : m_sentries)   if (e->alive) empHits++;
            for (auto& e : m_spawnerICE) if (e->alive) empHits++;
            for (auto& e : m_phantoms)   if (e->alive) empHits++;
            for (auto& e : m_leeches)    if (e->alive) empHits++;
            for (auto& e : m_mirrors)    if (e->alive) empHits++;
            if (empHits >= 5) {
                m_steam->unlockAchievement(ACH_EMP_MULTI);
                m_steam->checkCompletionist();
                m_empMultiUnlocked = true;
            }
        }
        break;
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
        float a      = m_avatar->angle;
        Vec2  dir    = Vec2::fromAngle(a);
        Vec2  bpos   = m_avatar->pos + dir * 20.f;
        auto  beam   = std::make_unique<Projectile>(bpos, dir * (Constants::PROJ_SPEED * 3.f), a);
        beam->lifetime = 2.0f;
        beam->pierce   = true;
        m_projectiles.push_back(std::move(beam));
        break;
    }
    case ProgramID::TWIN_SHOT:
        m_twinShotTimer = 10.0f;
        break;
    case ProgramID::BEACON:
        if (m_avatar) {
            m_beaconPos    = m_avatar->pos;
            m_beaconTimer  = 12.0f;
            m_beaconFireCd = 0.f;  // fire immediately on first tick
        }
        break;
    case ProgramID::NOVA_RING:
        m_novaTimer  = 10.0f;
        m_novaFireCd = 0.f;  // burst immediately
        break;
    case ProgramID::GRAVITY_WELL:
        m_gravTimer = 5.0f;
        break;
    default: break;
    }
}

// ---------------------------------------------------------------------------
// Objective
// ---------------------------------------------------------------------------

void CombatScene::checkObjective(SceneContext& ctx) {
    if (m_complete) return;

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
    case NodeObjective::Boss:    return;
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
// Collision / sweep helpers
// ---------------------------------------------------------------------------

void CombatScene::handleCollisions() {
    // Pre-state for achievement checks
    const bool stealthActive = (m_stealthTimer > 0.f);
    bool hasBounced = false;
    for (auto& p : m_projectiles)
        if (p->alive && p->bounced) { hasBounced = true; break; }

    const int preLiveICE = (int)(m_hunters.size() + m_sentries.size() + m_spawnerICE.size() +
                                 m_phantoms.size() + m_leeches.size() + m_mirrors.size());

    m_collision.update(m_avatar.get(), m_projectiles,
                       m_hunters, m_sentries, m_spawnerICE, m_phantoms, m_leeches, m_mirrors, m_enemyProjectiles);

    if (m_steam) {
        const int postLiveICE = (int)(m_hunters.size() + m_sentries.size() + m_spawnerICE.size() +
                                      m_phantoms.size() + m_leeches.size() + m_mirrors.size());
        if (postLiveICE < preLiveICE) {
            if (stealthActive && !m_stealthKillUnlocked) {
                m_steam->unlockAchievement(ACH_STEALTH_KILL);
                m_steam->checkCompletionist();
                m_stealthKillUnlocked = true;
            }
            if (hasBounced && !m_ricochetKillUnlocked) {
                for (auto& p : m_projectiles)
                    if (!p->alive && p->bounced) {
                        m_steam->unlockAchievement(ACH_RICOCHET_KILL);
                        m_steam->checkCompletionist();
                        m_ricochetKillUnlocked = true;
                        break;
                    }
            }
        }
    }
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
// Death effects
// ---------------------------------------------------------------------------

void CombatScene::emitDeathParticles() {
    auto emit = [this](const auto& vec, uint8_t r, uint8_t g, uint8_t b) {
        for (const auto& e : vec)
            if (!e->alive) m_particles.emit(e->pos, r, g, b, 14);
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
            m_fragments.emit(e->pos, Constants::COL_HUNTER_R, Constants::COL_HUNTER_G, Constants::COL_HUNTER_B, 10, 12.f, 400.f);
    for (const auto& e : m_sentries)
        if (!e->alive)
            m_fragments.emit(e->pos, Constants::COL_SENTRY_R, Constants::COL_SENTRY_G, Constants::COL_SENTRY_B, 13, 16.f, 310.f);
    for (const auto& e : m_spawnerICE)
        if (!e->alive)
            m_fragments.emit(e->pos, Constants::COL_SPAWNER_R, Constants::COL_SPAWNER_G, Constants::COL_SPAWNER_B, 18, 20.f, 280.f);
    for (const auto& e : m_phantoms)
        if (!e->alive)
            m_fragments.emit(e->pos, 50, 200, 220, 12, 14.f, 380.f);
    for (const auto& e : m_leeches)
        if (!e->alive)
            m_fragments.emit(e->pos, 180, 60, 255, 9, 12.f, 340.f);
    for (const auto& e : m_mirrors)
        if (!e->alive)
            m_fragments.emit(e->pos, 200, 230, 255, 12, 14.f, 320.f);
}
