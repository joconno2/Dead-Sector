#pragma once
#include <string>

// ---------------------------------------------------------------------------
// SteamManager — thin wrapper around Steamworks SDK.
//
// When built without USE_STEAM (-DUSE_STEAM=OFF, the default), all methods
// are no-ops and the Steamworks SDK is not required.
//
// When built with -DUSE_STEAM=ON:
//   1. Download the Steamworks SDK from https://partner.steamgames.com/
//   2. Copy the SDK's "public/" folder to deps/steamworks/public/
//   3. Copy the redistributable_bin/<platform>/ libraries to deps/steamworks/lib/
//   4. Set STEAM_APP_ID in steam_appid.txt (480 for testing with Spacewar)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Achievement API name constants — must match names registered in the
// Steamworks Partner dashboard under Achievements.
// ---------------------------------------------------------------------------
#define ACH_FIRST_KILL    "ACH_FIRST_KILL"    // Destroy your first ICE
#define ACH_FIRST_BOSS    "ACH_FIRST_BOSS"    // Defeat a boss node
#define ACH_CLEAR_WORLD1  "ACH_CLEAR_WORLD1"  // Complete SECTOR ALPHA
#define ACH_CLEAR_WORLD2  "ACH_CLEAR_WORLD2"  // Complete DEEP NET
#define ACH_FINAL_BREACH  "ACH_FINAL_BREACH"  // Complete THE CORE (final world)
#define ACH_GOLDEN_HULL   "ACH_GOLDEN_HULL"   // Earn a golden hull
#define ACH_HIGH_TRACE    "ACH_HIGH_TRACE"    // Reach 100% trace and survive

class SteamManager {
public:
    // Returns true if Steam was successfully initialised
    bool init();

    // Must be called every frame in the game loop (runs Steam callbacks)
    void tick();

    // Clean shutdown; call before SDL_Quit
    void shutdown();

    bool isAvailable()     const { return m_available; }

    // True while the Steam overlay is open — use to pause the game loop
    bool isOverlayActive() const { return m_overlayActive; }

    // Unlock a Steam achievement by API name (no-op if already unlocked or unavailable)
    void unlockAchievement(const char* apiName);

    // Steam Cloud save — wraps ISteamRemoteStorage.
    // cloudSave: upload 'contents' as "dead-sector.sav" in the Steam Cloud.
    // cloudLoad: download into 'out'; returns false if no cloud file exists.
    void cloudSave(const std::string& contents);
    bool cloudLoad(std::string& out);

    // Submit scores to Steam Leaderboards.
    // Uses FindLeaderboard → UploadLeaderboardScore async chain.
    // Replace leaderboard names with real names from Steamworks Partner.
    void submitNormalScore(int score);
    void submitEndlessScore(int score, int wave);

private:
    bool m_available     = false;
    bool m_overlayActive = false;

#ifdef STEAM_ENABLED
    // Pimpl — Steamworks SDK types stay out of the public header
    struct Impl;
    Impl* m_impl = nullptr;
#endif
};
