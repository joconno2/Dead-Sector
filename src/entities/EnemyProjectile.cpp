#include "EnemyProjectile.hpp"
#include "core/Constants.hpp"

EnemyProjectile::EnemyProjectile(Vec2 pos_, Vec2 vel_) {
    pos      = pos_;
    vel      = vel_;
    radius   = Constants::ENEMY_PROJ_RADIUS;
    lifetime = Constants::ENEMY_PROJ_LIFETIME;
    type     = EntityType::EnemyProjectile;

    // Small line segment like the player projectile
    m_localVerts = { {0.f, -3.f}, {0.f, 3.f} };
    transformVerts();
}

void EnemyProjectile::update(float dt) {
    lifetime -= dt;
    if (lifetime <= 0.f) alive = false;
}
