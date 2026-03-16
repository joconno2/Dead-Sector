#include "MirrorICE.hpp"
#include "core/Constants.hpp"
#include <cmath>

MirrorICE::MirrorICE(Vec2 pos_, Vec2 initialVel) {
    pos    = pos_;
    vel    = initialVel;
    radius = 18.f;
    type   = EntityType::MirrorICE;

    // Octagon
    constexpr int SIDES = 8;
    m_localVerts.resize(SIDES);
    for (int i = 0; i < SIDES; ++i) {
        float a = i * (2.f * 3.14159265f / SIDES);
        m_localVerts[i] = { std::cos(a) * radius, std::sin(a) * radius };
    }
    transformVerts();
}

void MirrorICE::update(float dt) {
    // Angle set externally by AISystem each frame (tracks player).
    // Body doesn't spin — it holds the facing angle.
    angle = facingAngle;
    if (hitFlashTimer > 0.f) hitFlashTimer -= dt;
}
