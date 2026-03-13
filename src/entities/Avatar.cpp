#include "Avatar.hpp"
#include "Projectile.hpp"
#include "core/Constants.hpp"
#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// Hull geometry definitions
// ---------------------------------------------------------------------------

std::vector<Vec2> hullVerts(HullType hull) {
    switch (hull) {

    case HullType::Raptor:
        // Signal Knife — swept interceptor, tip at bottom, swept wings above centre,
        // tapers to a clean closed handle at top.
        return {
            {  0.f,  34.f },   // needle tip (bottom)
            {  3.f,  24.f },   // right near-tip
            { 14.f,  32.f },   // right canard tip
            {  8.f,  14.f },   // right canard base
            { 28.f,  30.f },   // right main wing tip (swept)
            { 22.f,   2.f },   // right wing trailing root
            {  8.f,  -8.f },   // right hip
            {  5.f, -22.f },   // right engine pod
            {  0.f, -32.f },   // handle top — closed point
            { -5.f, -22.f },   // left engine pod
            { -8.f,  -8.f },   // left hip
            {-22.f,   2.f },   // left wing trailing root
            {-28.f,  30.f },   // left main wing tip
            { -8.f,  14.f },   // left canard base
            {-14.f,  32.f },   // left canard tip
            { -3.f,  24.f },   // left near-tip
        };

    case HullType::Mantis:
        // Praying mantis — claws extend well FORWARD of the head, narrow waist, wide hips.
        return {
            {  0.f, -22.f },   // head
            {  4.f, -16.f },   // right neck
            { 10.f, -20.f },   // right claw arm shoulder
            { 32.f, -34.f },   // right claw tip (far forward and wide)
            { 26.f,  -4.f },   // right claw elbow fold
            { 14.f,   6.f },   // right thorax shoulder
            {  5.f,  18.f },   // right hip
            {  9.f,  28.f },   // right engine
            {  3.f,  36.f },   // right exhaust nub
            {  0.f,  28.f },   // abdomen tail
            { -3.f,  36.f },   // left exhaust nub
            { -9.f,  28.f },   // left engine
            { -5.f,  18.f },   // left hip
            {-14.f,   6.f },   // left thorax shoulder
            {-26.f,  -4.f },   // left claw elbow fold
            {-32.f, -34.f },   // left claw tip
            {-10.f, -20.f },   // left claw arm shoulder
            { -4.f, -16.f },   // left neck
        };

    case HullType::Battle:
        // Coffin shape: very wide at the head/shoulders, sides angle inward,
        // noticeably narrower at the foot end — unmistakable funeral casket outline.
        return {
            {  7.f, -28.f },   // right head corner (flat top, close together)
            { 28.f, -18.f },   // right shoulder — widest point, like coffin shoulders
            { 30.f,  -4.f },   // right upper body
            { 24.f,  10.f },   // right waist (sides converge toward foot)
            { 16.f,  20.f },   // right lower body
            { 10.f,  26.f },   // right foot corner
            { 14.f,  34.f },   // right exhaust nub
            {  0.f,  30.f },   // foot center (narrower than head)
            {-14.f,  34.f },   // left exhaust nub
            {-10.f,  26.f },   // left foot corner
            {-16.f,  20.f },   // left lower body
            {-24.f,  10.f },   // left waist
            {-30.f,  -4.f },   // left upper body
            {-28.f, -18.f },   // left shoulder
            { -7.f, -28.f },   // left head corner
            // closes with flat top head edge — 14px wide at head vs ~28px at shoulders
        };

    case HullType::Blade:
        // Scalpel silhouette: extreme needle nose, almost zero body width,
        // one dramatic swept-delta wing spike mid-body, tiny split tail fins.
        return {
            {  0.f, -44.f },   // absurd razor tip
            {  1.f, -22.f },   // right near-nose (1px wide — nearly invisible from front)
            {  3.f,  -6.f },   // right body
            { 22.f,   6.f },   // right wing spike (larger, more dramatic delta)
            {  5.f,  16.f },   // right back to narrow waist
            {  4.f,  28.f },   // right tail fin root
            {  8.f,  36.f },   // right tail fin tip (split fins)
            {  2.f,  34.f },   // right tail fin inner
            {  0.f,  30.f },   // tail center notch
            { -2.f,  34.f },   // left tail fin inner
            { -8.f,  36.f },   // left tail fin tip
            { -4.f,  28.f },   // left tail fin root
            { -5.f,  16.f },   // left back to narrow waist
            {-22.f,   6.f },   // left wing spike
            { -3.f,  -6.f },   // left body
            { -1.f, -22.f },   // left near-nose
        };

    default: // Delta — original design, leave unchanged
        return {
            {  0.f, -30.f },   // sharp nose spike
            {  5.f, -11.f },   // right leading chine
            { 17.f,   5.f },   // right wing tip
            {  8.f,  11.f },   // right wing notch
            {  5.f,  17.f },   // right rear fin tip
            {  3.f,  25.f },   // right engine nub
            {  0.f,  19.f },   // tail indent
            { -3.f,  25.f },   // left engine nub
            { -5.f,  17.f },   // left rear fin tip
            { -8.f,  11.f },   // left wing notch
            {-17.f,   5.f },   // left wing tip
            { -5.f, -11.f },   // left leading chine
        };
    }
}

// ---------------------------------------------------------------------------
// Hull stats table
// ---------------------------------------------------------------------------

HullStats statsForHull(HullType hull) {
    switch (hull) {
    case HullType::Raptor: {
        HullStats s;
        s.id = "RAPTOR"; s.name = "SIGNAL KNIFE";
        s.flavor        = "Built for escape. Never meant to fight.";
        s.thrustMult    = 1.30f;
        s.speedMult     = 1.40f;
        s.rotMult       = 1.40f;
        s.projSpeedMult = 0.85f;
        s.radiusMult    = 0.90f;
        s.startingAmmo  = 20;
        s.runsRequired  = 3;
        return s;
    }
    case HullType::Mantis: {
        HullStats s;
        s.id = "MANTIS"; s.name = "STRIKE FRAME";
        s.flavor        = "Oversized railcannon bolted to a barely-legal hull.";
        s.thrustMult    = 0.85f;
        s.speedMult     = 0.85f;
        s.rotMult       = 0.85f;
        s.projSpeedMult = 1.50f;
        s.startingAmmo  = 40;
        s.killsRequired = 50;
        return s;
    }
    case HullType::Battle: {
        HullStats s;
        s.id = "BATTLE"; s.name = "IRON COFFIN";
        s.flavor        = "Walks slow. Takes everything. Eventually wins.";
        s.thrustMult    = 0.75f;
        s.speedMult     = 0.70f;
        s.rotMult       = 0.75f;
        s.projSpeedMult = 0.90f;
        s.radiusMult    = 1.10f;
        s.startingAmmo  = 50;
        s.extraLives    = 1;
        s.killsRequired = 200;
        return s;
    }
    case HullType::Blade: {
        HullStats s;
        s.id = "BLADE"; s.name = "GHOST WIRE";
        s.flavor        = "Kills in one pass. Counted corpses, never regrets.";
        s.thrustMult    = 1.50f;
        s.speedMult     = 1.60f;
        s.rotMult       = 1.60f;
        s.projSpeedMult = 1.20f;
        s.radiusMult    = 0.70f;
        s.startingAmmo  = 15;
        s.needsWin      = true;
        return s;
    }
    default: {
        HullStats s;
        s.id = "DELTA"; s.name = "GHOST RUNNER";
        s.flavor = "Standard-issue black-market vessel. No surprises.";
        return s;
    }
    }
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

Avatar::Avatar(float startX, float startY, HullType hull) {
    hullStats = statsForHull(hull);
    pos    = {startX, startY};
    radius = Constants::AVATAR_RADIUS * hullStats.radiusMult;
    type   = EntityType::Avatar;
    m_localVerts = hullVerts(hull);
    transformVerts();
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void Avatar::update(float dt) {
    if (thrusterTimer  > 0.f) thrusterTimer  -= dt;
    if (shieldTimer    > 0.f) { shieldTimer    -= dt; shielded = shieldTimer > 0.f; }
    if (overdriveTimer > 0.f)   overdriveTimer -= dt;
    thrusting = false;
}

// ---------------------------------------------------------------------------
// Movement
// ---------------------------------------------------------------------------

void Avatar::applyThrust(float dt, float forceMult, float speedCapMult) {
    Vec2 heading = Vec2::fromAngle(angle);
    vel += heading * (Constants::THRUST_FORCE * forceMult * dt);

    float cap = Constants::MAX_SPEED * speedCapMult;
    if (overdriveTimer > 0.f) cap *= 1.8f;
    float spd = vel.length();
    if (spd > cap)
        vel = vel * (cap / spd);

    thrusting     = true;
    thrusterTimer = 0.05f;
}

void Avatar::rotateLeft(float dt) {
    angle -= Constants::ROT_SPEED * dt;
}

void Avatar::rotateRight(float dt) {
    angle += Constants::ROT_SPEED * dt;
}

// ---------------------------------------------------------------------------
// Fire
// ---------------------------------------------------------------------------

Projectile* Avatar::fire(float speedMult, float radiusMult) {
    Vec2 heading  = Vec2::fromAngle(angle);
    Vec2 spawnPos = pos + heading * 18.f;
    Vec2 projVel  = heading * (Constants::PROJ_SPEED * speedMult);

    vel -= heading * Constants::RECOIL_IMPULSE;

    auto* p = new Projectile(spawnPos, projVel, angle);
    p->radius *= radiusMult;
    return p;
}
