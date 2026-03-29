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
// --- Progression ---
#define ACH_FIRST_KILL      "ACH_FIRST_KILL"      // Destroy your first ICE
#define ACH_FIRST_BOSS      "ACH_FIRST_BOSS"      // Defeat your first boss
#define ACH_CLEAR_WORLD1    "ACH_CLEAR_WORLD1"    // Complete SECTOR ALPHA
#define ACH_CLEAR_WORLD2    "ACH_CLEAR_WORLD2"    // Complete DEEP NET
#define ACH_FINAL_BREACH    "ACH_FINAL_BREACH"    // Complete THE CORE (final world)
#define ACH_GOLDEN_HULL     "ACH_GOLDEN_HULL"     // Earn a golden hull (world-3 clear)
#define ACH_HIGH_TRACE      "ACH_HIGH_TRACE"      // Reach 100% trace and survive

// --- Combat skill ---
#define ACH_KILL_MANTICORE  "ACH_KILL_MANTICORE"   // Defeat the Manticore (world 1 boss)
#define ACH_KILL_ARCHON     "ACH_KILL_ARCHON"      // Defeat the Archon (world 2 boss)
#define ACH_KILL_VORTEX     "ACH_KILL_VORTEX"      // Defeat the Vortex (world 3 boss)
#define ACH_CHAIN_KILL      "ACH_CHAIN_KILL"       // Kill 5 ICE within 10 seconds
#define ACH_RICOCHET_KILL   "ACH_RICOCHET_KILL"    // Kill an ICE with a bounced projectile
#define ACH_STEALTH_KILL    "ACH_STEALTH_KILL"     // Kill an ICE while STEALTH is active
#define ACH_EMP_MULTI       "ACH_EMP_MULTI"        // Hit 5+ ICE with a single EMP
#define ACH_SPEEDRUN        "ACH_SPEEDRUN"         // Kill a boss within 10s of spawning

// --- Mastery ---
#define ACH_ALL_HULLS       "ACH_ALL_HULLS"        // Unlock all 5 ship hulls
#define ACH_ALL_GOLDEN      "ACH_ALL_GOLDEN"       // Earn golden status on all 5 hulls
#define ACH_LEGENDARY_MOD   "ACH_LEGENDARY_MOD"   // Equip a legendary rarity mod

// --- Endless ---
#define ACH_ENDLESS_10      "ACH_ENDLESS_10"       // Survive 10 waves in endless mode
#define ACH_ENDLESS_25      "ACH_ENDLESS_25"       // Survive 25 waves in endless mode

// --- Completionist ---
#define ACH_FULL_BREACH     "ACH_FULL_BREACH"      // Unlock all 18 other achievements

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

    // Checks all 19 prerequisite achievements; unlocks ACH_FULL_BREACH if all done.
    // Call after any achievement unlock that might complete the set.
    void checkCompletionist();

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
