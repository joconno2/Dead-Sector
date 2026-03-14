#include "SaveSystem.hpp"
#include "Bindings.hpp"

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
        else if (key == "windowedHeight") data.windowedHeight = std::stoi(value);
        else if (key == "worldsUnlocked") data.worldsUnlocked = std::stoi(value);
        else if (key == "activeHull" && !value.empty()) data.activeHull = value;
        else if (key == "playerName" && !value.empty()) data.playerName = value;
        // Bindings — stored as "bind_<action>=<scancode int>"
        else if (key == "bind_thrust")   data.bindings.thrust   = (SDL_Scancode)std::stoi(value);
        else if (key == "bind_rotLeft")  data.bindings.rotLeft  = (SDL_Scancode)std::stoi(value);
        else if (key == "bind_rotRight") data.bindings.rotRight = (SDL_Scancode)std::stoi(value);
        else if (key == "bind_fire")     data.bindings.fire     = (SDL_Scancode)std::stoi(value);
        else if (key == "bind_prog0")    data.bindings.prog0    = (SDL_Scancode)std::stoi(value);
        else if (key == "bind_prog1")    data.bindings.prog1    = (SDL_Scancode)std::stoi(value);
        else if (key == "bind_prog2")    data.bindings.prog2    = (SDL_Scancode)std::stoi(value);
        // Controller button bindings
        else if (key == "bind_ctl_thrust")   data.bindings.ctlThrust   = (SDL_GameControllerButton)std::stoi(value);
        else if (key == "bind_ctl_rotLeft")  data.bindings.ctlRotLeft  = (SDL_GameControllerButton)std::stoi(value);
        else if (key == "bind_ctl_rotRight") data.bindings.ctlRotRight = (SDL_GameControllerButton)std::stoi(value);
        else if (key == "bind_ctl_fire")     data.bindings.ctlFire     = (SDL_GameControllerButton)std::stoi(value);
        else if (key == "bind_ctl_prog0")    data.bindings.ctlProg0    = (SDL_GameControllerButton)std::stoi(value);
        else if (key == "bind_ctl_prog1")    data.bindings.ctlProg1    = (SDL_GameControllerButton)std::stoi(value);
        else if (key == "bind_ctl_prog2")    data.bindings.ctlProg2    = (SDL_GameControllerButton)std::stoi(value);
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
        else if (key == "normalScore" && !value.empty()) {
            // format: NAME:SCORE
            auto sep = value.rfind(':');
            if (sep != std::string::npos && sep > 0) {
                ScoreEntry e;
                e.name  = value.substr(0, sep);
                try { e.score = std::stoi(value.substr(sep + 1)); } catch (...) {}
                data.normalScores.push_back(e);
            }
        }
        else if (key == "endlessScore" && !value.empty()) {
            // format: NAME:SCORE:WAVE
            auto c2 = value.rfind(':');
            if (c2 != std::string::npos && c2 > 0) {
                auto c1 = value.rfind(':', c2 - 1);
                if (c1 != std::string::npos) {
                    ScoreEntry e;
                    e.name  = value.substr(0, c1);
                    try {
                        e.score = std::stoi(value.substr(c1 + 1, c2 - c1 - 1));
                        e.wave  = std::stoi(value.substr(c2 + 1));
                    } catch (...) {}
                    data.endlessScores.push_back(e);
                }
            }
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
    f << "playerName="  << data.playerName  << "\n";
    f << "bind_thrust="   << (int)data.bindings.thrust   << "\n";
    f << "bind_rotLeft="  << (int)data.bindings.rotLeft  << "\n";
    f << "bind_rotRight=" << (int)data.bindings.rotRight << "\n";
    f << "bind_fire="     << (int)data.bindings.fire     << "\n";
    f << "bind_prog0="    << (int)data.bindings.prog0    << "\n";
    f << "bind_prog1="    << (int)data.bindings.prog1    << "\n";
    f << "bind_prog2="    << (int)data.bindings.prog2    << "\n";
    f << "bind_ctl_thrust="   << (int)data.bindings.ctlThrust   << "\n";
    f << "bind_ctl_rotLeft="  << (int)data.bindings.ctlRotLeft  << "\n";
    f << "bind_ctl_rotRight=" << (int)data.bindings.ctlRotRight << "\n";
    f << "bind_ctl_fire="     << (int)data.bindings.ctlFire     << "\n";
    f << "bind_ctl_prog0="    << (int)data.bindings.ctlProg0    << "\n";
    f << "bind_ctl_prog1="    << (int)data.bindings.ctlProg1    << "\n";
    f << "bind_ctl_prog2="    << (int)data.bindings.ctlProg2    << "\n";

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

    for (const auto& e : data.normalScores)
        f << "normalScore=" << e.name << ":" << e.score << "\n";

    for (const auto& e : data.endlessScores)
        f << "endlessScore=" << e.name << ":" << e.score << ":" << e.wave << "\n";
}
