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
    std::vector<std::unique_ptr<HunterICE>>&   hunters,
    std::vector<std::unique_ptr<SentryICE>>&   sentries,
    std::vector<std::unique_ptr<SpawnerICE>>&  spawners,
    std::vector<std::unique_ptr<PhantomICE>>&  phantoms,
    std::vector<std::unique_ptr<LeechICE>>&    leeches,
    std::vector<std::unique_ptr<MirrorICE>>&   mirrors,
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
    for (auto& ph : phantoms) if (ph->alive) updatePhantom(dt, *ph, *avatar, out);
    for (auto& lc : leeches)  if (lc->alive) updateLeech  (dt, *lc, *avatar, out);
    for (auto& mi : mirrors)  if (mi->alive) updateMirror (dt, *mi, *avatar);

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
// Phantom — cloaked; reveals near player, blinks before firing
// ---------------------------------------------------------------------------

void AISystem::updatePhantom(float dt, PhantomICE& ph, const Avatar& av, Result& out) {
    // Drift slowly toward player (half hunter speed)
    Vec2  toPlayer = av.pos - ph.pos;
    float dist     = toPlayer.length();
    if (dist > 0.001f) {
        Vec2 dir = toPlayer * (1.f / dist);
        ph.vel += dir * (Constants::HUNTER_ACCEL * 0.4f * dt);
        float spd = ph.vel.length();
        float maxSpd = Constants::HUNTER_MAX_SPEED * 0.55f;
        if (spd > maxSpd) ph.vel = ph.vel * (maxSpd / spd);
    }

    // Reveal when close enough
    ph.nearPlayer = (dist < PhantomICE::REVEAL_RADIUS);

    if (ph.blinkTimer > 0.f) {
        // --- BLINKING: warning flash before firing ---
        ph.blinkTimer -= dt;
        ph.visibility  = 1.f;  // fully visible while blinking
        if (ph.blinkTimer <= 0.f) {
            // Blink expired → fire, reset cooldown
            ph.blinkTimer   = 0.f;
            ph.fireCooldown = 2.5f;
            if (dist > 0.001f) {
                Vec2 dir     = toPlayer * (1.f / dist);
                Vec2 projPos = ph.pos + dir * (ph.radius + 5.f);
                Vec2 projVel = dir * Constants::ENEMY_PROJ_SPEED;
                out.firedProjectiles.push_back(std::make_unique<EnemyProjectile>(projPos, projVel));
            }
        }
    } else {
        // --- IDLE / CLOAKED: fade visibility, tick cooldown ---
        float targetVis = ph.nearPlayer ? 1.f : 0.f;
        ph.visibility  += (targetVis - ph.visibility) * std::min(1.f, dt * 4.f);

        ph.fireCooldown -= dt;
        if (ph.fireCooldown <= 0.f) {
            // Cooldown expired → start blink warning
            ph.blinkTimer = PhantomICE::BLINK_DURATION;
            ph.visibility = 1.f;
        }
    }
}

// ---------------------------------------------------------------------------
// Leech — seeks avatar, attaches and drains trace
// ---------------------------------------------------------------------------

void AISystem::updateLeech(float dt, LeechICE& lc, const Avatar& av, Result& out) {
    Vec2  toPlayer = av.pos - lc.pos;
    float dist     = toPlayer.length();

    if (!lc.attached) {
        // Chase at moderate speed
        if (dist > 0.001f) {
            Vec2 dir = toPlayer * (1.f / dist);
            lc.vel += dir * (Constants::HUNTER_ACCEL * 0.5f * dt);
            float spd = lc.vel.length();
            float maxSpd = Constants::HUNTER_MAX_SPEED * 0.65f;
            if (spd > maxSpd) lc.vel = lc.vel * (maxSpd / spd);
        }
        // Latch on when close enough
        if (dist < LeechICE::ATTACH_RADIUS + av.radius) {
            lc.attached = true;
            lc.vel      = {};   // stick
        }
    } else {
        // Ride the avatar — snap to its surface
        if (dist > 0.001f) {
            Vec2 dir = toPlayer * (1.f / dist);
            lc.pos   = av.pos - dir * (av.radius + lc.radius * 0.5f);
        }
        lc.vel = {};

        // Drain trace — report to caller
        out.leechTraceGain += LeechICE::LEECH_TRACE_RATE * dt;
    }
}

// ---------------------------------------------------------------------------
// Spawner — periodically creates Hunter ICE
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Mirror — faces player, slow drift toward them
// ---------------------------------------------------------------------------

void AISystem::updateMirror(float dt, MirrorICE& mi, const Avatar& av) {
    Vec2  toPlayer = av.pos - mi.pos;
    float dist     = toPlayer.length();
    if (dist > 0.001f) {
        // Track facing toward player
        mi.facingAngle = std::atan2(toPlayer.y, toPlayer.x);

        // Slow seek at ~30% hunter speed
        Vec2 dir = toPlayer * (1.f / dist);
        mi.vel += dir * (Constants::HUNTER_ACCEL * 0.25f * dt);
        float spd    = mi.vel.length();
        float maxSpd = Constants::HUNTER_MAX_SPEED * 0.3f;
        if (spd > maxSpd) mi.vel = mi.vel * (maxSpd / spd);
    }
}

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
