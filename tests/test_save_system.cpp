// tests/test_save_system.cpp — SaveSystem round-trip and SaveData logic tests
#include "core/SaveSystem.hpp"
#include <cassert>
#include <cstdio>
#include <iostream>

static int s_pass = 0, s_fail = 0;

#define CHECK(expr) \
    do { \
        if (expr) { ++s_pass; } \
        else { ++s_fail; std::cerr << "FAIL: " #expr " (" __FILE__ ":" << __LINE__ << ")\n"; } \
    } while(0)

// ---------------------------------------------------------------------------

static void test_defaults() {
    SaveData d;
    CHECK(d.credits    == 0);
    CHECK(d.totalRuns  == 0);
    CHECK(d.highScore  == 0);
    CHECK(d.activeHull == "DELTA");
    CHECK(d.musicVolume == 80);
    CHECK(d.sfxVolume   == 80);
    CHECK(d.worldsUnlocked == 1);
    CHECK(d.purchases.empty());
    CHECK(d.goldenHulls.empty());
    CHECK(d.playerName == "RUNNER");
}

static void test_purchase_count() {
    SaveData d;
    CHECK(d.purchaseCount("SCORE_BOOST") == 0);
    CHECK(!d.hasPurchase("SCORE_BOOST"));

    d.purchases.push_back("SCORE_BOOST");
    CHECK(d.purchaseCount("SCORE_BOOST") == 1);
    CHECK(d.hasPurchase("SCORE_BOOST"));

    d.purchases.push_back("SCORE_BOOST");
    CHECK(d.purchaseCount("SCORE_BOOST") == 2);

    CHECK(d.purchaseCount("OTHER") == 0);
}

static void test_golden_hulls() {
    SaveData d;
    CHECK(!d.isGolden("DELTA"));
    d.markGolden("DELTA");
    CHECK(d.isGolden("DELTA"));
    CHECK(!d.isGolden("NOVA"));
    d.markGolden("DELTA");  // duplicate — should not double-add
    d.markGolden("NOVA");
    CHECK(d.goldenHulls.size() == 2);
}

static void test_submit_score() {
    SaveData d;
    d.playerName = "TESTER";

    d.submitScore(false, 500);
    d.submitScore(false, 200);
    d.submitScore(false, 800);

    // Should be sorted descending
    CHECK(d.normalScores.size() == 3);
    CHECK(d.normalScores[0].score == 800);
    CHECK(d.normalScores[1].score == 500);
    CHECK(d.normalScores[2].score == 200);
    CHECK(d.normalScores[0].name  == "TESTER");

    // Cap at MAX_LEADERBOARD_ENTRIES
    for (int i = 0; i < MAX_LEADERBOARD_ENTRIES + 3; ++i)
        d.submitScore(false, i * 10);
    CHECK((int)d.normalScores.size() == MAX_LEADERBOARD_ENTRIES);

    // Endless scores are separate
    d.submitScore(true, 9999, 42);
    CHECK(d.endlessScores.size() == 1);
    CHECK(d.endlessScores[0].score == 9999);
    CHECK(d.endlessScores[0].wave  == 42);
    CHECK(d.normalScores.size()    == MAX_LEADERBOARD_ENTRIES); // unchanged
}

static void test_roundtrip() {
    const char* path = "/tmp/ds_test_save.sav";
    std::remove(path);

    SaveData out;
    out.credits        = 42;
    out.totalRuns      = 7;
    out.totalKills     = 123;
    out.highScore      = 9001;
    out.activeHull     = "NOVA";
    out.musicVolume    = 60;
    out.sfxVolume      = 40;
    out.worldsUnlocked = 2;
    out.playerName     = "NETRUNNER";
    out.purchases      = { "SCORE_BOOST", "SCORE_BOOST", "COOL_EXEC" };
    out.goldenHulls    = { "DELTA" };
    out.submitScore(false, 500);
    out.submitScore(true,  300, 5);

    SaveSystem::save(out, path);
    SaveData in = SaveSystem::load(path);

    CHECK(in.credits        == 42);
    CHECK(in.totalRuns      == 7);
    CHECK(in.totalKills     == 123);
    CHECK(in.highScore      == 9001);
    CHECK(in.activeHull     == "NOVA");
    CHECK(in.musicVolume    == 60);
    CHECK(in.sfxVolume      == 40);
    CHECK(in.worldsUnlocked == 2);
    CHECK(in.playerName     == "NETRUNNER");
    CHECK(in.purchaseCount("SCORE_BOOST") == 2);
    CHECK(in.purchaseCount("COOL_EXEC")   == 1);
    CHECK(in.isGolden("DELTA"));
    CHECK(!in.isGolden("NOVA"));
    CHECK(in.normalScores.size()  == 1);
    CHECK(in.normalScores[0].score == 500);
    CHECK(in.endlessScores.size() == 1);
    CHECK(in.endlessScores[0].score == 300);
    CHECK(in.endlessScores[0].wave  == 5);

    std::remove(path);
}

static void test_load_missing_file() {
    // Loading a non-existent file should return defaults silently
    SaveData d = SaveSystem::load("/tmp/ds_does_not_exist_abc123.sav");
    CHECK(d.credits == 0);
    CHECK(d.activeHull == "DELTA");
}

// ---------------------------------------------------------------------------

int main() {
    test_defaults();
    test_purchase_count();
    test_golden_hulls();
    test_submit_score();
    test_roundtrip();
    test_load_missing_file();

    std::cout << "SaveSystem: " << s_pass << " passed, " << s_fail << " failed\n";
    return s_fail == 0 ? 0 : 1;
}
