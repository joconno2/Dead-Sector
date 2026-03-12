#include "HunterICE.hpp"
#include "core/Constants.hpp"

HunterICE::HunterICE(Vec2 pos_, Vec2 initialVel) {
    pos    = pos_;
    vel    = initialVel;
    radius = Constants::HUNTER_RADIUS;
    type   = EntityType::HunterICE;

    // Elongated triangle — tip forward (up in local space)
    m_localVerts = {
        {  0.f, -18.f },  // tip
        {  9.f,  12.f },  // right rear
        { -9.f,  12.f },  // left rear
    };
    transformVerts();
}

void HunterICE::update(float dt) {
    // Steering is applied by AISystem before physics integration.
    // Rotate to face velocity direction.
    float spd = vel.length();
    if (spd > 5.f) {
        angle = std::atan2(vel.x, -vel.y);
    }
}
