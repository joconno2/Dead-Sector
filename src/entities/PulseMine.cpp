#include "PulseMine.hpp"
#include <cmath>

PulseMine::PulseMine(float x, float y) {
    pos    = { x, y };
    vel    = { 0.f, 0.f };
    radius = 10.f;
    type   = EntityType::PulseMine;
    noWrap = true; // stationary, never wraps

    // Hexagon body (distinct from sentry diamond)
    const int N = 6;
    for (int i = 0; i < N; ++i) {
        float a = i * 6.28318f / N - 3.14159f / N; // flat-top hex
        m_localVerts.push_back({ std::cos(a) * 11.f, std::sin(a) * 11.f });
    }
    transformVerts();
}

void PulseMine::update(float dt) {
    if (!alive) return;

    m_timer += dt;

    switch (m_state) {
    case PulseState::Idle:
        if (m_timer >= IDLE_TIME) {
            m_state = PulseState::Warning;
            m_timer = 0.f;
            m_pulseRadius = 0.f;
        }
        break;

    case PulseState::Warning:
        // Expanding warning ring
        m_pulseRadius = (m_timer / WARN_TIME) * DAMAGE_RADIUS;
        if (m_timer >= WARN_TIME) {
            m_state = PulseState::Pulse;
            m_timer = 0.f;
            m_pulseRadius = DAMAGE_RADIUS;
        }
        break;

    case PulseState::Pulse:
        if (m_timer >= PULSE_TIME) {
            m_state = PulseState::Idle;
            m_timer = 0.f;
            m_pulseRadius = 0.f;
        }
        break;

    case PulseState::Dead:
        alive = false;
        break;
    }
}

float PulseMine::warningFrac() const {
    if (m_state != PulseState::Warning) return 0.f;
    return m_timer / WARN_TIME;
}
