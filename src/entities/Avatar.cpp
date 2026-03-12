#include "Avatar.hpp"
#include "Projectile.hpp"
#include "core/Constants.hpp"
#include <cmath>

Avatar::Avatar(float startX, float startY) {
    pos    = {startX, startY};
    radius = Constants::AVATAR_RADIUS;
    type   = EntityType::Avatar;
    // Aggressive delta-wing silhouette
    m_localVerts = {
        {  0.f, -30.f },   // sharp nose spike
        {  5.f, -11.f },   // right leading chine
        { 17.f,   5.f },   // right wing tip
        {  8.f,  11.f },   // right wing notch (angular delta cutout)
        {  5.f,  17.f },   // right rear fin tip
        {  3.f,  25.f },   // right engine nub
        {  0.f,  19.f },   // tail indent
        { -3.f,  25.f },   // left engine nub
        { -5.f,  17.f },   // left rear fin tip
        { -8.f,  11.f },   // left wing notch
        {-17.f,   5.f },   // left wing tip
        { -5.f, -11.f },   // left leading chine
    };
    transformVerts();
}

void Avatar::update(float dt) {
    if (thrusterTimer  > 0.f) thrusterTimer  -= dt;
    if (shieldTimer    > 0.f) { shieldTimer    -= dt; shielded = shieldTimer > 0.f; }
    if (overdriveTimer > 0.f)   overdriveTimer -= dt;
    thrusting = false; // reset each frame; set by CombatScene if thrust input active
}

void Avatar::applyThrust(float dt, float forceMult, float speedCapMult) {
    Vec2 heading = Vec2::fromAngle(angle);
    vel += heading * (Constants::THRUST_FORCE * forceMult * dt);

    // Clamp to max speed (INERTIA_DAMP raises the cap, overdrive raises it further)
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

Projectile* Avatar::fire(float speedMult, float radiusMult) {
    Vec2 heading  = Vec2::fromAngle(angle);
    Vec2 spawnPos = pos + heading * 18.f;
    Vec2 projVel  = heading * (Constants::PROJ_SPEED * speedMult);

    vel -= heading * Constants::RECOIL_IMPULSE;

    auto* p = new Projectile(spawnPos, projVel, angle);
    p->radius *= radiusMult;
    return p;
}
