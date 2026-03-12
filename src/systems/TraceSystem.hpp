#pragma once
#include <functional>

// Manages the Trace % danger meter.
// Trace rises passively over time and spikes on hits/alarms.
// Fires a callback each time a new threshold (25/50/75/100) is first crossed.
class TraceSystem {
public:
    using ThresholdCallback = std::function<void(int thresholdPct)>;

    explicit TraceSystem(float tickRate = 1.8f);

    void update(float dt);

    void onHit();          // +TRACE_HIT_PENALTY %
    void onAlarm();        // +TRACE_ALARM_ADD %
    void add(float delta); // direct adjustment (can be negative)

    void reset();

    float trace()     const { return m_trace; }
    int   threshold() const { return m_crossedThreshold; } // highest crossed: 0/25/50/75/100

    void setThresholdCallback(ThresholdCallback cb) { m_callback = std::move(cb); }
    void setTickRate(float r) { m_tickRate = r; }

private:
    float m_trace             = 0.f;
    float m_tickRate;           // %/sec
    int   m_crossedThreshold  = 0; // 0 means none crossed yet

    ThresholdCallback m_callback;

    void clampAndCheckThresholds();
};
