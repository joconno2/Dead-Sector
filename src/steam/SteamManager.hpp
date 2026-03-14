#pragma once

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

class SteamManager {
public:
    // Returns true if Steam was successfully initialised
    bool init();

    // Must be called every frame in the game loop (runs Steam callbacks)
    void tick();

    // Clean shutdown; call before SDL_Quit
    void shutdown();

    bool isAvailable() const { return m_available; }

    // Submit scores to Steam Leaderboards.
    // These are no-ops until you have a real AppID and leaderboard names.
    void submitNormalScore(int score);
    void submitEndlessScore(int score, int wave);

private:
    bool m_available = false;

#ifdef STEAM_ENABLED
    // Leaderboard find/upload state (used by Steamworks callback machinery)
    // Defined in SteamManager.cpp when STEAM_ENABLED
    struct Impl;
    Impl* m_impl = nullptr;
#endif
};
