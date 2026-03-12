#include "SentryICE.hpp"
#include "core/Constants.hpp"

SentryICE::SentryICE(Vec2 pos_, Vec2 driftVel) {
    pos    = pos_;
    vel    = driftVel;
    radius = Constants::SENTRY_RADIUS;
    type   = EntityType::SentryICE;

    // Square / diamond
    m_localVerts = {
        {  0.f, -18.f },  // top
        { 16.f,   0.f },  // right
        {  0.f,  18.f },  // bottom
        {-16.f,   0.f },  // left
    };
    transformVerts();
}

void SentryICE::update(float /*dt*/) {
    // AISystem handles rotation and fire trigger.
    // A slow constant spin when not tracking would go here if needed.
}
