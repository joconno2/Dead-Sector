#pragma once
#include "Entity.hpp"

// Leech ICE — does not damage the avatar directly.
// Instead it latches on when close, slowing the avatar slightly and
// draining trace at +LEECH_TRACE_RATE %/sec while attached.
// Shooting it off destroys it; it does not respawn from a kill.
class LeechICE : public Entity {
public:
    explicit LeechICE(Vec2 pos, Vec2 initialVel = {});
    void update(float dt) override;
    int  scoreValue() const override { return 5; }

    bool  attached    = false;   // true once latched to avatar
    float traceTimer  = 0.f;     // accumulates while attached for periodic trace spikes

    static constexpr float ATTACH_RADIUS    = 22.f;  // latch distance
    static constexpr float LEECH_TRACE_RATE = 6.f;   // extra %/sec while attached
};
