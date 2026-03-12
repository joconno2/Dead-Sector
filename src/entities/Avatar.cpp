#include "Avatar.hpp"
#include "Projectile.hpp"
#include "core/Constants.hpp"
#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// Hull geometry definitions
// ---------------------------------------------------------------------------

static std::vector<Vec2> hullVerts(HullType hull) {
    switch (hull) {
    case HullType::Raptor:
        // Swept-wing interceptor — aggressive forward-swept delta
        return {
            {  0.f, -35.f },   // needle nose
            {  4.f, -13.f },   // leading chine right
            { 22.f,   0.f },   // right wing tip (far swept)
            { 12.f,   9.f },   // right wing root notch
            {  5.f,  20.f },   // right rear fin
            {  3.f,  28.f },   // right engine nub
            {  0.f,  22.f },   // tail indent
            { -3.f,  28.f },   // left engine nub
            { -5.f,  20.f },   // left rear fin
            {-12.f,   9.f },   // left wing root notch
            {-22.f,   0.f },   // left wing tip
            { -4.f, -13.f },   // leading chine left
        };
    case HullType::Mantis:
        // Insectoid forward-claws
        return {
            {  0.f, -28.f },   // nose
            {  8.f, -20.f },   // right claw inner
            { 22.f, -26.f },   // right claw tip (forward-pointing)
            { 18.f,  -8.f },   // right claw base
            { 14.f,   6.f },   // right mid
            {  5.f,  24.f },   // right engine
            {  2.f,  30.f },   // right nub
            {  0.f,  22.f },   // tail center
            { -2.f,  30.f },   // left nub
            { -5.f,  24.f },   // left engine
            {-14.f,   6.f },   // left mid
            {-18.f,  -8.f },   // left claw base
            {-22.f, -26.f },   // left claw tip
            { -8.f, -20.f },   // left claw inner
        };
    case HullType::Blade:
        // Ultra-thin knife silhouette
        return {
            {  0.f, -38.f },   // very sharp nose
            {  3.f,  -6.f },   // right chine
            { 10.f,   8.f },   // right cut
            {  4.f,  18.f },   // right rear
            {  2.f,  30.f },   // right nub
            {  0.f,  24.f },   // tail
            { -2.f,  30.f },   // left nub
            { -4.f,  18.f },   // left rear
            {-10.f,   8.f },   // left cut
            { -3.f,  -6.f },   // left chine
        };
    case HullType::Battle:
        // Heavy armored delta — wide, angular, imposing
        return {
            {  0.f, -24.f },   // nose
            {  9.f, -18.f },   // right shoulder
            { 24.f,  -4.f },   // right wing tip (wide)
            { 24.f,   8.f },   // right wing trailing edge
            { 15.f,  14.f },   // right flank
            {  8.f,  22.f },   // right engine pod
            {  4.f,  28.f },   // right nub
            {  0.f,  24.f },   // tail indent
            { -4.f,  28.f },   // left nub
            { -8.f,  22.f },   // left engine pod
            {-15.f,  14.f },   // left flank
            {-24.f,   8.f },   // left wing trailing edge
            {-24.f,  -4.f },   // left wing tip
            { -9.f, -18.f },   // left shoulder
        };
    default: // Delta — original design
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
// Constructor
// ---------------------------------------------------------------------------

Avatar::Avatar(float startX, float startY, HullType hull) {
    pos    = {startX, startY};
    radius = Constants::AVATAR_RADIUS;
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
