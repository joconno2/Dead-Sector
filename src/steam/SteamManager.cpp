#include "SteamManager.hpp"

#ifdef STEAM_ENABLED
// ---------------------------------------------------------------------------
// Full Steamworks build
// ---------------------------------------------------------------------------
#include "steam/steam_api.h"
#include <iostream>

bool SteamManager::init() {
    if (!SteamAPI_Init()) {
        std::cerr << "[Steam] SteamAPI_Init failed — Steam not running or wrong AppID\n";
        m_available = false;
        return false;
    }
    m_available = true;
    std::cerr << "[Steam] Initialised. AppID: " << SteamUtils()->GetAppID() << "\n";
    return true;
}

void SteamManager::tick() {
    if (m_available) SteamAPI_RunCallbacks();
}

void SteamManager::shutdown() {
    if (m_available) {
        SteamAPI_Shutdown();
        m_available = false;
    }
}

void SteamManager::submitNormalScore(int score) {
    if (!m_available) return;
    // TODO: replace "Dead_Sector_Normal" with the actual leaderboard name from Steamworks Partner
    // SteamUserStats()->FindOrCreateLeaderboard("Dead_Sector_Normal", ...);
    (void)score;
}

void SteamManager::submitEndlessScore(int score, int wave) {
    if (!m_available) return;
    // TODO: replace "Dead_Sector_Endless" with the actual leaderboard name
    (void)score; (void)wave;
}

#else
// ---------------------------------------------------------------------------
// Stub build — no Steamworks SDK required
// ---------------------------------------------------------------------------

bool SteamManager::init()     { return false; }
void SteamManager::tick()     {}
void SteamManager::shutdown() {}
void SteamManager::submitNormalScore(int)        {}
void SteamManager::submitEndlessScore(int, int)  {}

#endif  // STEAM_ENABLED
