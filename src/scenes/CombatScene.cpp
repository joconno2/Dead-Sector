#include "CombatScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "GameOverScene.hpp"
#include "NodeCompleteScene.hpp"
#include "core/Constants.hpp"
#include "systems/ModSystem.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "math/Vec2.hpp"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <limits>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

CombatScene::CombatScene(NodeConfig cfg) : m_config(cfg) {}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void CombatScene::onEnter(SceneContext& ctx) {
    // Reset program cooldowns so all slots start ready
    if (ctx.programs) {
        ctx.programs->resetCooldowns();
        if (ctx.mods) ctx.programs->setCooldownMultiplier(ctx.mods->cdMult());
    }
    setupCollisionCallback(ctx);
    resetGame();

    // Node-entry mod effects
    if (ctx.mods) {
        if (m_avatar) m_avatar->extraLives = ctx.mods->startingExtraLives();
        if (ctx.mods->hasGhostProtocol())
            m_trace.setTickRate(m_config.traceTickRate * 0.7f);
    }
}

void CombatScene::onExit() {
    m_projectiles.clear();
    m_hunters.clear();
    m_sentries.clear();
    m_spawnerICE.clear();
    m_enemyProjectiles.clear();
    m_pendingHunters.clear();
    m_avatar.reset();
}

void CombatScene::setupCollisionCallback(SceneContext& ctx) {
    SceneManager* scenes = ctx.scenes;
    ModSystem*    mods   = ctx.mods;
    m_collision.setCallback([this, scenes, mods](const CollisionEvent& ev) {
        if (ev.type == CollisionEvent::Type::ProjectileHitICE) {
            if (!ev.projectile->alive || !ev.iceEntity->alive) return;

            // PHANTOM_ROUND: shot always pierces (projectile stays alive)
            // CRIT_MATRIX: 20% chance shot survives the kill
            bool pierce = false;
            if (mods) {
                if (mods->hasPhantomRound()) pierce = true;
                else if (mods->hasCritMatrix()) {
                    static thread_local std::mt19937 rng(std::random_device{}());
                    std::uniform_real_distribution<float> d(0.f, 1.f);
                    if (d(rng) < 0.20f) pierce = true;
                }
            }
            if (!pierce) ev.projectile->alive = false;
            ev.iceEntity->alive = false;
            m_score += ev.iceEntity->scoreValue();
            m_iceKilled++;

            // TRACE_SINK: each kill reduces trace
            if (mods) m_trace.add(-mods->traceSinkAmt());

            // PHASE_FRAME: each kill briefly makes avatar invulnerable
            if (mods && mods->hasPhaseFrame() && m_avatar && m_avatar->alive) {
                m_avatar->shieldTimer = std::max(m_avatar->shieldTimer, 0.3f);
                m_avatar->shielded    = true;
            }

        } else if (ev.type == CollisionEvent::Type::AvatarHitICE) {
            if (!ev.avatar->alive) return;
            if (ev.avatar->shielded) { ev.iceEntity->alive = false; return; }
            if (ev.avatar->extraLives > 0) {
                // ADAPTIVE_ARMOR: absorb the hit, brief invincibility flash
                ev.iceEntity->alive = false;
                ev.avatar->extraLives--;
                ev.avatar->shieldTimer = 0.8f;
                ev.avatar->shielded    = true;
                m_trace.onHit();
                return;
            }
            ev.avatar->alive    = false;
            ev.iceEntity->alive = false;
            m_trace.onHit();
            scenes->replace(std::make_unique<GameOverScene>(m_score));

        } else if (ev.type == CollisionEvent::Type::EnemyProjectileHitAvatar) {
            if (!ev.avatar->alive || !ev.enemyProj->alive) return;
            if (ev.avatar->shielded) { ev.enemyProj->alive = false; return; }
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
            scenes->replace(std::make_unique<GameOverScene>(m_score));
        }
    });
}

void CombatScene::resetGame() {
    m_score        = 0;
    m_iceKilled    = 0;
    m_complete     = false;
    m_firePrev     = false;
    m_prog0Prev = m_prog1Prev = m_prog2Prev = false;
    m_empTimer     = 0.f;
    m_stealthTimer = 0.f;
    m_surviveTimer = m_config.surviveSeconds;

    m_projectiles.clear();
    m_hunters.clear();
    m_sentries.clear();
    m_spawnerICE.clear();
    m_enemyProjectiles.clear();
    m_pendingHunters.clear();

    m_particles.clear();
    m_trace.reset();
    m_trace.setTickRate(m_config.traceTickRate);
    m_spawner.reset();

    m_avatar = std::make_unique<Avatar>(
        Constants::SCREEN_WF * 0.5f, Constants::SCREEN_HF * 0.5f);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void CombatScene::handleEvent(SDL_Event& ev, SceneContext&) { (void)ev; }

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void CombatScene::update(float dt, SceneContext& ctx) {
    if (m_complete) return;

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    m_input.thrustForward = keys[SDL_SCANCODE_UP]    || keys[SDL_SCANCODE_W];
    m_input.rotLeft       = keys[SDL_SCANCODE_LEFT]  || keys[SDL_SCANCODE_A];
    m_input.rotRight      = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
    m_input.fire          = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_LCTRL];
    m_input.prog0         = keys[SDL_SCANCODE_Q];
    m_input.prog1         = keys[SDL_SCANCODE_E];
    m_input.prog2         = keys[SDL_SCANCODE_R];

    // Controller input — OR into existing keyboard state
    if (ctx.controller) {
        constexpr Sint16 AXIS_DEAD = 8000;
        Sint16 lx = SDL_GameControllerGetAxis(ctx.controller, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 rt = SDL_GameControllerGetAxis(ctx.controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

        if (lx < -AXIS_DEAD || SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
            m_input.rotLeft = true;
        if (lx >  AXIS_DEAD || SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
            m_input.rotRight = true;

        // Right bumper or D-pad up = thrust button
        if (SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
         || SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_DPAD_UP))
            m_input.thrustForward = true;

        // Right trigger or A button fires
        if (rt > AXIS_DEAD || SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_A))
            m_input.fire = true;

        // Face buttons activate programs (edge detected via prev flags below)
        if (SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_X)) m_input.prog0 = true;
        if (SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_Y)) m_input.prog1 = true;
        if (SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_B)) m_input.prog2 = true;
    }

    // Mod multipliers for this frame
    float thrustMult  = ctx.mods ? ctx.mods->thrustMult()    : 1.f;
    float rotMult     = ctx.mods ? ctx.mods->rotMult()       : 1.f;
    float speedCapMult= ctx.mods ? ctx.mods->maxSpeedMult()  : 1.f;
    float pSpeedMult  = ctx.mods ? ctx.mods->projSpeedMult() : 1.f;
    float pRadMult    = ctx.mods ? ctx.mods->projRadiusMult(): 1.f;
    bool  splitRound  = ctx.mods ? ctx.mods->hasSplitRound() : false;
    bool  overcharge  = ctx.mods ? ctx.mods->hasOvercharge()  : false;

    if (m_avatar && m_avatar->alive) {
        if (m_input.rotLeft)       m_avatar->rotateLeft(dt  * rotMult);
        if (m_input.rotRight)      m_avatar->rotateRight(dt * rotMult);
        if (m_input.thrustForward) m_avatar->applyThrust(dt, thrustMult, speedCapMult);

        // Exhaust particles — emitted from both engine nubs, flickering cyan-white
        if (m_input.thrustForward) {
            Vec2 heading    = Vec2::fromAngle(m_avatar->angle);
            Vec2 perp       = { -heading.y, heading.x };
            Vec2 exhaustDir = { -heading.x, -heading.y };   // backward
            Vec2 eR = m_avatar->pos - heading * 16.f + perp * 2.f;
            Vec2 eL = m_avatar->pos - heading * 16.f - perp * 2.f;
            m_particles.emitThrust(eR, exhaustDir, 80, 200, 255, 6);
            m_particles.emitThrust(eL, exhaustDir, 80, 200, 255, 6);
        }

        bool rapidFire = (m_avatar->overdriveTimer > 0.f);
        bool fireEdge  = m_input.fire && (!m_firePrev || rapidFire);
        if (fireEdge) {
            Projectile* p = m_avatar->fire(pSpeedMult, pRadMult);
            if (p) {
                m_projectiles.emplace_back(p);
                m_shotCount++;

                // SPLIT_ROUND: every 4th shot fires a parallel twin
                if (splitRound && m_shotCount % 4 == 0) {
                    Vec2 h    = Vec2::fromAngle(m_avatar->angle);
                    Vec2 perp = { -h.y, h.x };
                    Vec2 twinPos = m_avatar->pos + h * 18.f + perp * 10.f;
                    Vec2 twinVel = h * (Constants::PROJ_SPEED * pSpeedMult);
                    auto twin = std::make_unique<Projectile>(twinPos, twinVel, m_avatar->angle);
                    twin->radius *= pRadMult;
                    m_projectiles.push_back(std::move(twin));
                }

                // OVERCHARGE: every 6th shot fires a 3-round angular burst
                if (overcharge && m_shotCount % 6 == 0) {
                    Vec2 h = Vec2::fromAngle(m_avatar->angle);
                    constexpr float offsets[] = { -0.28f, 0.f, 0.28f };
                    for (float off : offsets) {
                        float a   = m_avatar->angle + off;
                        Vec2 dir  = Vec2::fromAngle(a);
                        Vec2 bpos = m_avatar->pos + dir * 18.f;
                        Vec2 bvel = dir * (Constants::PROJ_SPEED * pSpeedMult);
                        auto burst = std::make_unique<Projectile>(bpos, bvel, a);
                        burst->radius *= pRadMult;
                        m_projectiles.push_back(std::move(burst));
                    }
                    (void)h;
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
    if (m_empTimer     > 0.f) m_empTimer     -= dt;
    if (m_stealthTimer > 0.f) m_stealthTimer -= dt;

    // Physics
    std::vector<Entity*> all;
    all.reserve(1 + m_projectiles.size() + m_hunters.size()
                  + m_sentries.size() + m_spawnerICE.size() + m_enemyProjectiles.size());
    if (m_avatar && m_avatar->alive) all.push_back(m_avatar.get());
    for (auto& p  : m_projectiles)      all.push_back(p.get());
    for (auto& h  : m_hunters)          all.push_back(h.get());
    for (auto& s  : m_sentries)         all.push_back(s.get());
    for (auto& sp : m_spawnerICE)       all.push_back(sp.get());
    for (auto& ep : m_enemyProjectiles) all.push_back(ep.get());
    m_physics.update(dt, all);

    // AI — skipped while EMP active
    if (m_empTimer <= 0.f) {
        auto aiResult = m_ai.update(dt, m_hunters, m_sentries, m_spawnerICE, m_avatar.get());
        for (auto& ep : aiResult.firedProjectiles)
            m_enemyProjectiles.push_back(std::move(ep));
        for (auto& h : aiResult.spawnedHunters)
            m_pendingHunters.push_back(std::move(h));
    }

    handleCollisions();

    for (auto& h : m_pendingHunters)
        m_hunters.push_back(std::move(h));
    m_pendingHunters.clear();

    emitDeathParticles();
    sweepDead();

    m_particles.update(dt);

    if (m_stealthTimer <= 0.f) m_trace.update(dt);

    int liveCount = static_cast<int>(m_hunters.size() + m_sentries.size() + m_spawnerICE.size());
    auto batch = m_spawner.update(dt, m_trace.trace(), liveCount);
    for (auto& h  : batch.hunters)    m_hunters.push_back(std::move(h));
    for (auto& s  : batch.sentries)   m_sentries.push_back(std::move(s));
    for (auto& sp : batch.spawnerICE) m_spawnerICE.push_back(std::move(sp));

    if (m_config.objective == NodeObjective::Survive) m_surviveTimer -= dt;

    checkObjective(ctx);
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
            Vec2 vel     = heading * Constants::PROJ_SPEED;
            m_projectiles.emplace_back(std::make_unique<Projectile>(pos, vel, a));
        }
        break;
    }
    case ProgramID::EMP:      m_empTimer     = 2.0f; break;
    case ProgramID::STEALTH:  m_stealthTimer = 8.0f; break;
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
        if (nearest) { nearest->alive = false; m_score += nearest->scoreValue(); m_iceKilled++; }
        break;
    }
    case ProgramID::FEEDBACK: {
        if (!m_avatar) break;
        constexpr float R = 150.f;
        auto burst = [&](auto& vec) {
            for (auto& e : vec) {
                if (e->alive && (e->pos - m_avatar->pos).length() <= R) {
                    e->alive = false; m_score += e->scoreValue(); m_iceKilled++;
                }
            }
        };
        burst(m_hunters); burst(m_sentries); burst(m_spawnerICE);
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
    bool done = false;
    switch (m_config.objective) {
    case NodeObjective::Sweep:
    case NodeObjective::Boss:
        done = (m_iceKilled >= m_config.sweepTarget); break;
    case NodeObjective::Survive:
        done = (m_surviveTimer <= 0.f); break;
    }
    if (done) {
        m_complete = true;
        ctx.scenes->replace(std::make_unique<NodeCompleteScene>(
            m_config.nodeId, m_score, m_iceKilled, m_config.sweepTarget));
    }
}

// ---------------------------------------------------------------------------
// Collision / sweep
// ---------------------------------------------------------------------------

void CombatScene::handleCollisions() {
    m_collision.update(m_avatar.get(), m_projectiles,
                       m_hunters, m_sentries, m_spawnerICE, m_enemyProjectiles);
}

void CombatScene::sweepDead() {
    auto dead = [](const auto& e) { return !e->alive; };
    m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(), dead), m_projectiles.end());
    m_hunters.erase(std::remove_if(m_hunters.begin(), m_hunters.end(), dead), m_hunters.end());
    m_sentries.erase(std::remove_if(m_sentries.begin(), m_sentries.end(), dead), m_sentries.end());
    m_spawnerICE.erase(std::remove_if(m_spawnerICE.begin(), m_spawnerICE.end(), dead), m_spawnerICE.end());
    m_enemyProjectiles.erase(std::remove_if(m_enemyProjectiles.begin(), m_enemyProjectiles.end(), dead), m_enemyProjectiles.end());
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void CombatScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    vr->clear();

    // Per-node grid color: tier1=blue, tier2=green, tier3=amber, tier4=red
    uint8_t gr, gg, gb;
    switch (m_config.tier) {
        case 2:  gr=20;  gg=80;  gb=50;  break;  // green
        case 3:  gr=80;  gg=50;  gb=20;  break;  // amber
        case 4:  gr=80;  gg=20;  gb=50;  break;  // red/magenta
        default: gr=20;  gg=50;  gb=80;  break;  // blue (tier 1)
    }
    vr->drawGrid(gr, gg, gb);

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

    if (m_avatar && m_avatar->alive) {
        vr->drawGlowPoly(m_avatar->worldVerts(), avatarColor);
        if (m_avatar->thrusting) {
            Vec2  heading = Vec2::fromAngle(m_avatar->angle);
            Vec2  perp    = { -heading.y, heading.x };
            float flicker = 8.f + 10.f * ((float)(SDL_GetTicks() % 120) / 120.f);
            Vec2 eR = m_avatar->pos - heading * 14.f + perp * 3.f;
            Vec2 eL = m_avatar->pos - heading * 14.f - perp * 3.f;
            vr->drawGlowLine(eR, eR - heading * flicker, avatarColor);
            vr->drawGlowLine(eL, eL - heading * flicker, avatarColor);
        }
    }

    // EMP tint
    if (m_empTimer > 0.f) {
        SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ctx.renderer, 0, 200, 200, 18);
        SDL_RenderFillRect(ctx.renderer, nullptr);
    }

    // Particles — drawn before CRT overlay so scanlines sit on top
    m_particles.render(ctx.renderer);

    vr->drawCRTOverlay();

    // Glitch effect: kicks in above 75% trace
    float glitchIntensity = std::max(0.f, (m_trace.trace() - 75.f) / 25.f);
    if (glitchIntensity > 0.f) vr->drawGlitch(glitchIntensity, SDL_GetTicks());

    if (ctx.hud) ctx.hud->render(m_score, m_trace.trace(), ctx.programs, ctx.mods);

    // Objective display — bottom center
    if (ctx.hud) {
        std::string objStr;
        if (m_config.objective == NodeObjective::Survive) {
            int s = std::max(0, (int)m_surviveTimer + 1);
            objStr = "SURVIVE: " + std::to_string(s) + "s";
        } else {
            objStr = "ICE: " + std::to_string(m_iceKilled)
                   + " / " + std::to_string(m_config.sweepTarget);
        }
        SDL_Color cyan = { 100, 220, 200, 220 };
        int approxW = (int)objStr.size() * 12;
        ctx.hud->drawLabel(objStr, Constants::SCREEN_W / 2 - approxW / 2,
                           Constants::SCREEN_H - 38, cyan);
    }

    SDL_RenderPresent(ctx.renderer);
}

void CombatScene::renderICE(SceneContext& ctx) const {
    VectorRenderer* vr = ctx.vrenderer;
    GlowColor hunterColor  = { Constants::COL_HUNTER_R,  Constants::COL_HUNTER_G,  Constants::COL_HUNTER_B  };
    GlowColor sentryColor  = { Constants::COL_SENTRY_R,  Constants::COL_SENTRY_G,  Constants::COL_SENTRY_B  };
    GlowColor spawnerColor = { Constants::COL_SPAWNER_R, Constants::COL_SPAWNER_G, Constants::COL_SPAWNER_B };
    GlowColor eprojColor   = { Constants::COL_EPROJ_R,   Constants::COL_EPROJ_G,   Constants::COL_EPROJ_B   };

    for (auto& h  : m_hunters)  vr->drawGlowPoly(h->worldVerts(), hunterColor);
    for (auto& s  : m_sentries) vr->drawGlowPoly(s->worldVerts(), sentryColor);
    for (auto& sp : m_spawnerICE) {
        vr->drawGlowPoly(sp->worldVerts(), spawnerColor);
        const auto& v = sp->worldVerts();
        std::vector<Vec2> inner;
        inner.reserve(v.size());
        for (const auto& pt : v)
            inner.push_back(sp->pos + (pt - sp->pos) * 0.5f);
        vr->drawGlowPoly(inner, spawnerColor);
    }
    for (auto& ep : m_enemyProjectiles) {
        const auto& v = ep->worldVerts();
        if (v.size() >= 2) vr->drawGlowLine(v[0], v[1], eprojColor);
    }
}

// ---------------------------------------------------------------------------
// Particles
// ---------------------------------------------------------------------------

void CombatScene::emitDeathParticles() {
    auto emit = [this](const auto& vec, uint8_t r, uint8_t g, uint8_t b) {
        for (const auto& e : vec)
            if (!e->alive) m_particles.emit(e->pos, r, g, b, 12);
    };
    emit(m_hunters,          Constants::COL_HUNTER_R,  Constants::COL_HUNTER_G,  Constants::COL_HUNTER_B);
    emit(m_sentries,         Constants::COL_SENTRY_R,  Constants::COL_SENTRY_G,  Constants::COL_SENTRY_B);
    emit(m_spawnerICE,       Constants::COL_SPAWNER_R, Constants::COL_SPAWNER_G, Constants::COL_SPAWNER_B);
    emit(m_enemyProjectiles, Constants::COL_EPROJ_R,   Constants::COL_EPROJ_G,   Constants::COL_EPROJ_B);
}
