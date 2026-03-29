// tests/test_trace_system.cpp — TraceSystem tick, thresholds, callbacks
#include "systems/TraceSystem.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>

static int s_pass = 0, s_fail = 0;

#define CHECK(expr) \
    do { \
        if (expr) { ++s_pass; } \
        else { ++s_fail; std::cerr << "FAIL: " #expr " (" __FILE__ ":" << __LINE__ << ")\n"; } \
    } while(0)

#define CHECK_NEAR(a, b, eps) CHECK(std::abs((float)(a) - (float)(b)) < (float)(eps))

// ---------------------------------------------------------------------------

static void test_initial_state() {
    TraceSystem ts;
    CHECK_NEAR(ts.trace(), 0.0f, 0.001f);
    CHECK(ts.threshold() == 0);
}

static void test_reset() {
    TraceSystem ts(2.0f);
    ts.update(10.0f);  // drive trace up
    CHECK(ts.trace() > 0.0f);
    ts.reset();
    CHECK_NEAR(ts.trace(), 0.0f, 0.001f);
    CHECK(ts.threshold() == 0);
}

static void test_tick_rate() {
    // At tickRate=10%/sec, after 5s trace should be ~50%
    TraceSystem ts(10.0f);
    ts.update(5.0f);
    CHECK_NEAR(ts.trace(), 50.0f, 0.5f);
}

static void test_clamp_at_100() {
    TraceSystem ts(100.0f);
    ts.update(10.0f);
    CHECK(ts.trace() <= 100.0f);
}

static void test_add_positive() {
    TraceSystem ts(0.0f);  // no passive tick
    ts.add(30.0f);
    CHECK_NEAR(ts.trace(), 30.0f, 0.001f);
}

static void test_add_negative() {
    TraceSystem ts(0.0f);
    ts.add(60.0f);
    ts.add(-20.0f);
    CHECK_NEAR(ts.trace(), 40.0f, 0.001f);
}

static void test_clamp_below_zero() {
    TraceSystem ts(0.0f);
    ts.add(-50.0f);
    CHECK_NEAR(ts.trace(), 0.0f, 0.001f);
}

static void test_on_hit() {
    TraceSystem ts(0.0f);
    float before = ts.trace();
    ts.onHit();
    CHECK(ts.trace() > before);
}

static void test_threshold_callbacks() {
    TraceSystem ts(0.0f);
    std::vector<int> fired;
    ts.setThresholdCallback([&](int pct) { fired.push_back(pct); });

    ts.add(21.0f);   // crosses 20
    CHECK(fired.size() == 1);
    CHECK(fired[0] == 20);

    ts.add(20.0f);   // crosses 40
    CHECK(fired.size() == 2);
    CHECK(fired[1] == 40);

    ts.add(20.0f);   // crosses 60
    CHECK(fired.size() == 3);
    CHECK(fired[2] == 60);

    ts.add(20.0f);   // crosses 80
    CHECK(fired.size() == 4);
    CHECK(fired[3] == 80);

    ts.add(20.0f);   // crosses 100
    CHECK(fired.size() == 5);
    CHECK(fired[4] == 100);

    // Thresholds should not fire again once crossed
    ts.add(1.0f);
    CHECK(fired.size() == 5);
}

static void test_threshold_not_repeated_after_reset() {
    TraceSystem ts(0.0f);
    std::vector<int> fired;
    ts.setThresholdCallback([&](int pct) { fired.push_back(pct); });

    ts.add(25.0f);   // crosses 20
    CHECK(fired.size() == 1);

    ts.reset();
    fired.clear();

    // After reset, thresholds should fire again
    ts.add(25.0f);
    CHECK(fired.size() == 1);
    CHECK(fired[0] == 20);
}

static void test_threshold_accessor() {
    TraceSystem ts(0.0f);
    CHECK(ts.threshold() == 0);
    ts.add(25.0f);
    CHECK(ts.threshold() == 20);
    ts.add(25.0f);
    CHECK(ts.threshold() == 40);
}

static void test_set_tick_rate() {
    TraceSystem ts(0.0f);
    ts.setTickRate(20.0f);
    ts.update(3.0f);
    CHECK_NEAR(ts.trace(), 60.0f, 0.5f);
}

// ---------------------------------------------------------------------------

int main() {
    test_initial_state();
    test_reset();
    test_tick_rate();
    test_clamp_at_100();
    test_add_positive();
    test_add_negative();
    test_clamp_below_zero();
    test_on_hit();
    test_threshold_callbacks();
    test_threshold_not_repeated_after_reset();
    test_threshold_accessor();
    test_set_tick_rate();

    std::cout << "TraceSystem: " << s_pass << " passed, " << s_fail << " failed\n";
    return s_fail == 0 ? 0 : 1;
}
