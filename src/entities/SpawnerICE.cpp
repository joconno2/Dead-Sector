#include "SpawnerICE.hpp"
#include "core/Constants.hpp"
#include <cmath>

SpawnerICE::SpawnerICE(Vec2 pos_, Vec2 vel_) {
    pos    = pos_;
    vel    = vel_;
    radius = Constants::SPAWNER_RADIUS;
    type   = EntityType::SpawnerICE;

    // Hexagon
    constexpr float R = 22.f;
    for (int i = 0; i < 6; ++i) {
        float a = i * (2.f * 3.14159265f / 6.f);
        m_localVerts.push_back({ std::sin(a) * R, -std::cos(a) * R });
    }
    transformVerts();
}

void SpawnerICE::update(float dt) {
    angle += 0.6f * dt;  // slow rotation
}
