#pragma once
#include <string>
#include <vector>
#include <algorithm>

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
    std::vector<std::string> purchases;   // shop item IDs already bought
    std::vector<std::string> goldenHulls; // hulls that completed a run victory

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
};

class SaveSystem {
public:
    static constexpr const char* SAVE_PATH = "dead-sector.sav";
    static SaveData load(const std::string& path = SAVE_PATH);
    static void     save(const SaveData& data, const std::string& path = SAVE_PATH);
};
