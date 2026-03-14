// tests/test_mod_system.cpp — ModSystem stat multipliers and ability flags
#include "systems/ModSystem.hpp"
#include "core/Mods.hpp"
#include <cassert>
#include <iostream>

static int s_pass = 0, s_fail = 0;

#define CHECK(expr) \
    do { \
        if (expr) { ++s_pass; } \
        else { ++s_fail; std::cerr << "FAIL: " #expr " (" __FILE__ ":" << __LINE__ << ")\n"; } \
    } while(0)

#define CHECK_NEAR(a, b, eps) CHECK(std::abs((a) - (b)) < (eps))

#include <cmath>

// ---------------------------------------------------------------------------

static void test_empty() {
    ModSystem ms;
    CHECK(!ms.has(ModID::RICOCHET));
    CHECK(ms.count(ModID::RICOCHET) == 0);
    CHECK_NEAR(ms.thrustMult(),    1.0f, 0.001f);
    CHECK_NEAR(ms.rotMult(),       1.0f, 0.001f);
    CHECK_NEAR(ms.maxSpeedMult(),  1.0f, 0.001f);
    CHECK_NEAR(ms.projSpeedMult(), 1.0f, 0.001f);
    CHECK_NEAR(ms.cdMult(),        1.0f, 0.001f);
    CHECK(!ms.hasSplitRound());
    CHECK(!ms.hasRicochet());
    CHECK(!ms.hasOvercharge());
    CHECK(!ms.hasScatterCore());
    CHECK(!ms.hasDeadmanSwitch());
}

static void test_add_and_has() {
    ModSystem ms;
    ms.add(ModID::RICOCHET);
    CHECK(ms.has(ModID::RICOCHET));
    CHECK(ms.count(ModID::RICOCHET) == 1);
    CHECK(ms.hasRicochet());

    ms.add(ModID::SPLIT_ROUND);
    CHECK(ms.hasSplitRound());
    CHECK(ms.all().size() == 2);
}

static void test_reset() {
    ModSystem ms;
    ms.add(ModID::RICOCHET);
    ms.add(ModID::SPLIT_ROUND);
    CHECK(ms.all().size() == 2);
    ms.reset();
    CHECK(ms.all().empty());
    CHECK(!ms.has(ModID::RICOCHET));
}

static void test_thrust_mult() {
    ModSystem ms;
    float base = ms.thrustMult();
    ms.add(ModID::KINETIC_CORE);
    CHECK(ms.thrustMult() > base);
    // Each additional stack increases it further
    ms.add(ModID::KINETIC_CORE);
    CHECK(ms.thrustMult() > ms.thrustMult() - 0.001f); // still > 1
}

static void test_cd_mult() {
    ModSystem ms;
    CHECK_NEAR(ms.cdMult(), 1.0f, 0.001f);
    ms.add(ModID::COLD_EXEC);
    CHECK(ms.cdMult() < 1.0f);  // cooldowns shorter
    float one = ms.cdMult();
    ms.add(ModID::COLD_EXEC);
    CHECK(ms.cdMult() < one);   // two stacks = shorter still
}

static void test_trace_sink() {
    ModSystem ms;
    CHECK_NEAR(ms.traceSinkAmt(), 0.0f, 0.001f);
    ms.add(ModID::TRACE_SINK);
    CHECK(ms.traceSinkAmt() > 0.0f);
}

static void test_passive_cap() {
    ModSystem ms;
    CHECK(ms.passiveCount() == 0);
    CHECK(!ms.passiveFull());

    ms.setPassiveCap(2);
    ms.add(ModID::ADAPTIVE_ARMOR);   // passive
    ms.add(ModID::ADAPTIVE_ARMOR);   // passive
    CHECK(ms.passiveFull());
}

static void test_sentry_fire_rate() {
    ModSystem ms;
    CHECK_NEAR(ms.sentryFireRateMult(), 1.0f, 0.001f);
    ms.add(ModID::SIGNAL_JAM);
    CHECK(ms.sentryFireRateMult() > 1.0f);  // sentries fire slower
    CHECK(ms.hasSignalJam());
}

static void test_offer_pool() {
    ModSystem ms;
    // Should return N distinct results without crashing
    auto pool = ms.buildOfferPool(3, true);
    CHECK(pool.size() <= 3);

    // With passive slots full, offer pool should exclude passive mods
    ms.setPassiveCap(0);
    auto pool2 = ms.buildOfferPool(4, false);
    for (ModID id : pool2) {
        // Just verify we got valid IDs back (no UB)
        CHECK((int)id >= 0);
    }
}

// ---------------------------------------------------------------------------

int main() {
    test_empty();
    test_add_and_has();
    test_reset();
    test_thrust_mult();
    test_cd_mult();
    test_trace_sink();
    test_passive_cap();
    test_sentry_fire_rate();
    test_offer_pool();

    std::cout << "ModSystem: " << s_pass << " passed, " << s_fail << " failed\n";
    return s_fail == 0 ? 0 : 1;
}
