#pragma once
#include "Entity.hpp"

enum class PulseState { Idle, Warning, Pulse, Dead };

class PulseMine : public Entity {
public:
    PulseMine(float x, float y);
    void update(float dt) override;
    int  scoreValue() const override { return 150; }

    PulseState state() const { return m_state; }
    float      pulseRadius() const { return m_pulseRadius; }
    float      warningFrac() const; // 0-1 during warning phase
    int        hitsLeft   = 3;

    // Damage radius when pulse fires
    static constexpr float DAMAGE_RADIUS = 130.f;

private:
    PulseState m_state       = PulseState::Idle;
    float      m_timer       = 0.f;
    float      m_pulseRadius = 0.f;

    static constexpr float IDLE_TIME    = 3.5f;
    static constexpr float WARN_TIME    = 1.0f;
    static constexpr float PULSE_TIME   = 0.15f;
};
