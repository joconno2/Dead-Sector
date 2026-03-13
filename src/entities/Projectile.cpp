#include "Projectile.hpp"
#include "core/Constants.hpp"

Projectile::Projectile(Vec2 startPos, Vec2 startVel, float startAngle) {
    pos    = startPos;
    vel    = startVel;
    angle  = startAngle;
    radius = 7.f;
    type   = EntityType::Projectile;
    noWrap = true;  // bullets fizzle at screen edges instead of wrapping

    // A longer bright dash
    m_localVerts = { {0.f, -7.f}, {0.f, 7.f} };
    transformVerts();
}

void Projectile::update(float dt) {
    lifetime -= dt;
    if (lifetime <= 0.f) { alive = false; return; }
    // Fizzle when leaving the screen
    if (pos.x < 0.f || pos.x > Constants::SCREEN_WF ||
        pos.y < 0.f || pos.y > Constants::SCREEN_HF)
        alive = false;
}
