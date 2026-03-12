#pragma once
#include <string>
#include <vector>
#include <algorithm>

struct SaveData {
    int  credits    = 0;
    int  totalRuns  = 0;
    int  totalKills = 0;
    int  highScore  = 0;
    std::string activeHull = "DELTA"; // ship hull cosmetic ID
    std::vector<std::string> purchases; // shop item IDs already bought

    bool hasPurchase(const std::string& id) const {
        return std::find(purchases.begin(), purchases.end(), id) != purchases.end();
    }
    int purchaseCount(const std::string& id) const {
        return (int)std::count(purchases.begin(), purchases.end(), id);
    }
};

class SaveSystem {
public:
    static constexpr const char* SAVE_PATH = "dead-sector.sav";
    static SaveData load(const std::string& path = SAVE_PATH);
    static void     save(const SaveData& data, const std::string& path = SAVE_PATH);
};
