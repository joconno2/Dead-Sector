#include "LeechICE.hpp"
#include "core/Constants.hpp"
#include <cmath>

LeechICE::LeechICE(Vec2 pos_, Vec2 initialVel) {
    pos    = pos_;
    vel    = initialVel;
    radius = 10.f;
    type   = EntityType::LeechICE;

    // Ring of tendrils: alternating long (14) / short (5) spikes — parasitic look
    const int POINTS = 6;
    for (int i = 0; i < POINTS * 2; ++i) {
        float a = i * 3.14159265f / POINTS;
        float r = (i % 2 == 0) ? 14.f : 5.f;
        m_localVerts.push_back({ std::cos(a) * r, std::sin(a) * r });
    }
    transformVerts();
}

void LeechICE::update(float dt) {
    // Slowly spin when chasing
    if (!attached) angle += 1.5f * dt;
    // When attached, spin faster (visually clings)
    else           angle += 3.f * dt;
}
