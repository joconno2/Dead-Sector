#include "PhantomICE.hpp"
#include "core/Constants.hpp"
#include <cmath>

PhantomICE::PhantomICE(Vec2 pos_, Vec2 initialVel) {
    pos    = pos_;
    vel    = initialVel;
    radius = 14.f;
    type   = EntityType::PhantomICE;

    // Diamond with inner cross — ghostly four-pointed shape
    const int POINTS = 4;
    for (int i = 0; i < POINTS * 2; ++i) {
        float a = i * 3.14159265f / POINTS - 3.14159265f / 4.f;
        float r = (i % 2 == 0) ? 16.f : 6.f;
        m_localVerts.push_back({ std::cos(a) * r, std::sin(a) * r });
    }
    transformVerts();
}

void PhantomICE::update(float dt) {
    // Visibility and blinkTimer managed by AISystem; only rotate here.
    angle += 0.8f * dt;
}
