#include "SentryICE.hpp"
#include "core/Constants.hpp"
#include <cmath>

SentryICE::SentryICE(Vec2 pos_, Vec2 driftVel) {
    pos    = pos_;
    vel    = driftVel;
    radius = Constants::SENTRY_RADIUS;
    type   = EntityType::SentryICE;

    // 8-pointed star: alternating outer (20) / inner (8) radii — looks like a turret/cannon
    const int POINTS = 8;
    for (int i = 0; i < POINTS * 2; ++i) {
        float a = i * 3.14159265f / POINTS;
        float r = (i % 2 == 0) ? 20.f : 8.f;
        m_localVerts.push_back({ std::cos(a) * r, std::sin(a) * r });
    }
    transformVerts();
}

void SentryICE::update(float /*dt*/) {
    // AISystem handles rotation and fire trigger.
}
