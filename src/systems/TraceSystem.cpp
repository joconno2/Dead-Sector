#include "TraceSystem.hpp"
#include "core/Constants.hpp"
#include <algorithm>

static constexpr int THRESHOLDS[] = { 25, 50, 75, 100 };

TraceSystem::TraceSystem(float tickRate)
    : m_tickRate(tickRate)
{}

void TraceSystem::update(float dt) {
    m_trace += m_tickRate * dt;
    clampAndCheckThresholds();
}

void TraceSystem::onHit() {
    m_trace += Constants::TRACE_HIT_PENALTY;
    clampAndCheckThresholds();
}

void TraceSystem::onAlarm() {
    m_trace += Constants::TRACE_ALARM_ADD;
    clampAndCheckThresholds();
}

void TraceSystem::add(float delta) {
    m_trace += delta;
    if (m_trace < 0.f) m_trace = 0.f;
    clampAndCheckThresholds();
}

void TraceSystem::reset() {
    m_trace            = 0.f;
    m_crossedThreshold = 0;
}

void TraceSystem::clampAndCheckThresholds() {
    m_trace = std::min(m_trace, 100.f);

    for (int t : THRESHOLDS) {
        if (m_trace >= static_cast<float>(t) && m_crossedThreshold < t) {
            m_crossedThreshold = t;
            if (m_callback) m_callback(t);
        }
    }
}
