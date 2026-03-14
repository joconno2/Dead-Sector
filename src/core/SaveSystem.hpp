#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "Bindings.hpp"

static constexpr int MAX_LEADERBOARD_ENTRIES = 10;

struct ScoreEntry {
    std::string name;
    int         score = 0;
    int         wave  = 0;  // endless: wave reached; 0 for normal runs
};

struct SaveData {
    int  credits    = 0;
    int  totalRuns  = 0;
    int  totalKills = 0;
    int  highScore  = 0;
    std::string activeHull = "DELTA"; // ship hull selected for next run
    int  musicVolume = 80;            // 0-100
    int  sfxVolume   = 80;            // 0-100
    int  displayMode    = 2;    // 0=windowed, 1=fullscreen, 2=borderless (default: borderless)
    int  windowedWidth  = 1280;
    int  windowedHeight = 720;
    int  worldsUnlocked = 1;    // number of worlds accessible as run start points (1–3)
    std::vector<std::string> purchases;   // shop item IDs already bought
    std::vector<std::string> goldenHulls; // hulls that completed a world-3 run victory

    // Key bindings
    Bindings                bindings;

    // Leaderboard
    std::string             playerName   = "RUNNER";
    std::vector<ScoreEntry> normalScores;    // top MAX_LEADERBOARD_ENTRIES, descending
    std::vector<ScoreEntry> endlessScores;   // top MAX_LEADERBOARD_ENTRIES, descending

    bool hasPurchase(const std::string& id) const {
        return std::find(purchases.begin(), purchases.end(), id) != purchases.end();
    }
    int purchaseCount(const std::string& id) const {
        return (int)std::count(purchases.begin(), purchases.end(), id);
    }
    bool isGolden(const std::string& hullId) const {
        return std::find(goldenHulls.begin(), goldenHulls.end(), hullId) != goldenHulls.end();
    }
    void markGolden(const std::string& hullId) {
        if (!isGolden(hullId)) goldenHulls.push_back(hullId);
    }

    // Insert a finished run score; keeps list sorted and capped at MAX_LEADERBOARD_ENTRIES
    void submitScore(bool endless, int score, int wave = 0) {
        auto& list = endless ? endlessScores : normalScores;
        ScoreEntry e; e.name = playerName; e.score = score; e.wave = wave;
        list.push_back(e);
        std::sort(list.begin(), list.end(),
                  [](const ScoreEntry& a, const ScoreEntry& b){ return a.score > b.score; });
        if ((int)list.size() > MAX_LEADERBOARD_ENTRIES)
            list.resize(MAX_LEADERBOARD_ENTRIES);
    }
};

class SaveSystem {
public:
    static constexpr const char* SAVE_PATH = "dead-sector.sav";
    static SaveData load(const std::string& path = SAVE_PATH);
    static void     save(const SaveData& data, const std::string& path = SAVE_PATH);
};
