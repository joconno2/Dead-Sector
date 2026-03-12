#include "Projectile.hpp"

Projectile::Projectile(Vec2 startPos, Vec2 startVel, float startAngle) {
    pos    = startPos;
    vel    = startVel;
    angle  = startAngle;
    radius = 7.f;
    type   = EntityType::Projectile;

    // A longer bright dash
    m_localVerts = { {0.f, -7.f}, {0.f, 7.f} };
    transformVerts();
}

void Projectile::update(float dt) {
    lifetime -= dt;
    if (lifetime <= 0.f) alive = false;
}
