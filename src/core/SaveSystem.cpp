#include "SaveSystem.hpp"

#include <fstream>
#include <sstream>

SaveData SaveSystem::load(const std::string& path) {
    SaveData data;
    std::ifstream f(path);
    if (!f.is_open()) return data;

    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key   = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        if      (key == "credits")     data.credits     = std::stoi(value);
        else if (key == "totalRuns")   data.totalRuns   = std::stoi(value);
        else if (key == "totalKills")  data.totalKills  = std::stoi(value);
        else if (key == "highScore")   data.highScore   = std::stoi(value);
        else if (key == "musicVolume") data.musicVolume = std::stoi(value);
        else if (key == "sfxVolume")   data.sfxVolume   = std::stoi(value);
        else if (key == "displayMode")    data.displayMode    = std::stoi(value);
        else if (key == "windowedWidth")  data.windowedWidth  = std::stoi(value);
        else if (key == "windowedHeight")  data.windowedHeight  = std::stoi(value);
        else if (key == "worldsUnlocked")  data.worldsUnlocked  = std::stoi(value);
        else if (key == "activeHull" && !value.empty()) data.activeHull = value;
        else if (key == "purchases"  && !value.empty()) {
            std::istringstream ss(value);
            std::string item;
            while (std::getline(ss, item, ','))
                if (!item.empty()) data.purchases.push_back(item);
        }
        else if (key == "goldenHulls" && !value.empty()) {
            std::istringstream ss(value);
            std::string item;
            while (std::getline(ss, item, ','))
                if (!item.empty()) data.goldenHulls.push_back(item);
        }
    }
    return data;
}

void SaveSystem::save(const SaveData& data, const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << "credits="     << data.credits     << "\n";
    f << "totalRuns="   << data.totalRuns   << "\n";
    f << "totalKills="  << data.totalKills  << "\n";
    f << "highScore="   << data.highScore   << "\n";
    f << "musicVolume=" << data.musicVolume << "\n";
    f << "sfxVolume="   << data.sfxVolume   << "\n";
    f << "displayMode="    << data.displayMode    << "\n";
    f << "windowedWidth="  << data.windowedWidth  << "\n";
    f << "windowedHeight=" << data.windowedHeight << "\n";
    f << "worldsUnlocked=" << data.worldsUnlocked << "\n";
    f << "activeHull="  << data.activeHull  << "\n";

    f << "purchases=";
    for (int i = 0; i < (int)data.purchases.size(); ++i) {
        if (i > 0) f << ",";
        f << data.purchases[i];
    }
    f << "\n";

    f << "goldenHulls=";
    for (int i = 0; i < (int)data.goldenHulls.size(); ++i) {
        if (i > 0) f << ",";
        f << data.goldenHulls[i];
    }
    f << "\n";
}
