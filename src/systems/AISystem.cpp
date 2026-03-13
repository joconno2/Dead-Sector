#include "AISystem.hpp"
#include "entities/Avatar.hpp"
#include "core/Constants.hpp"
#include <cmath>
#include <random>

static std::mt19937 s_rng(std::random_device{}());

// ---------------------------------------------------------------------------
// Top-level update
// ---------------------------------------------------------------------------

AISystem::Result AISystem::update(
    float dt,
    std::vector<std::unique_ptr<HunterICE>>&  hunters,
    std::vector<std::unique_ptr<SentryICE>>&  sentries,
    std::vector<std::unique_ptr<SpawnerICE>>& spawners,
    const Avatar* avatar,
    AIConfig cfg)
{
    Result out;
    if (!avatar || !avatar->alive) return out;

    // Hunters target the decoy when CLONE is active, otherwise avatar
    Vec2 hunterTarget = cfg.decoyActive ? cfg.decoyPos : avatar->pos;

    for (auto& h  : hunters)  if (h->alive)  updateHunter (dt, *h, hunterTarget);
    for (auto& s  : sentries) if (s->alive)  updateSentry (dt, *s, *avatar, out, cfg.sentryFireRateMult);
    for (auto& sp : spawners) if (sp->alive) updateSpawner(dt, *sp, out);

    return out;
}

// ---------------------------------------------------------------------------
// Hunter — accelerates toward target position (avatar or decoy)
// ---------------------------------------------------------------------------

void AISystem::updateHunter(float dt, HunterICE& h, Vec2 targetPos) {
    Vec2  toTarget = targetPos - h.pos;
    float dist     = toTarget.length();
    if (dist < 0.001f) return;

    Vec2 dir = toTarget * (1.f / dist);
    h.vel += dir * (Constants::HUNTER_ACCEL * dt);

    float spd = h.vel.length();
    if (spd > Constants::HUNTER_MAX_SPEED)
        h.vel = h.vel * (Constants::HUNTER_MAX_SPEED / spd);
}

// ---------------------------------------------------------------------------
// Sentry — rotates to aim at player, fires when cooldown expires
// ---------------------------------------------------------------------------

void AISystem::updateSentry(float dt, SentryICE& s, const Avatar& av, Result& out,
                             float fireRateMult) {
    Vec2  toPlayer    = av.pos - s.pos;
    float targetAngle = std::atan2(toPlayer.x, -toPlayer.y);

    float diff = targetAngle - s.angle;
    while (diff >  3.14159265f) diff -= 6.28318530f;
    while (diff < -3.14159265f) diff += 6.28318530f;

    float step = Constants::SENTRY_ROT_SPEED * dt;
    s.angle += (std::abs(diff) < step) ? diff : std::copysign(step, diff);

    // fire rate multiplier >1 = slower (SIGNAL_JAM: 1.667)
    s.fireCooldown -= dt;
    if (s.fireCooldown <= 0.f) {
        s.fireCooldown = Constants::SENTRY_FIRE_RATE * fireRateMult;

        constexpr float SPREAD = 0.30f;
        for (int k = -1; k <= 1; ++k) {
            float a       = s.angle + k * SPREAD;
            Vec2  heading = Vec2::fromAngle(a);
            Vec2  projPos = s.pos + heading * (s.radius + 5.f);
            Vec2  projVel = heading * Constants::SENTRY_PROJ_SPEED;
            out.firedProjectiles.push_back(std::make_unique<EnemyProjectile>(projPos, projVel));
        }
    }
}

// ---------------------------------------------------------------------------
// Spawner — periodically creates Hunter ICE
// ---------------------------------------------------------------------------

void AISystem::updateSpawner(float dt, SpawnerICE& sp, Result& out) {
    sp.spawnCooldown -= dt;
    if (sp.spawnCooldown <= 0.f && sp.spawnedCount < SpawnerICE::MAX_CHILDREN) {
        sp.spawnCooldown = Constants::SPAWNER_SPAWN_RATE;
        ++sp.spawnedCount;

        std::uniform_real_distribution<float> angleDist(0.f, 6.2831853f);
        float a        = angleDist(s_rng);
        Vec2  dir      = { std::cos(a), std::sin(a) };
        Vec2  spawnPos = sp.pos + dir * (sp.radius + 10.f);
        Vec2  vel      = dir * (Constants::HUNTER_MAX_SPEED * 0.45f);

        out.spawnedHunters.push_back(std::make_unique<HunterICE>(spawnPos, vel));
    }
}
