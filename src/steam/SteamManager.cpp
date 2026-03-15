#include "SteamManager.hpp"

#ifdef STEAM_ENABLED
// ---------------------------------------------------------------------------
// Full Steamworks build
// ---------------------------------------------------------------------------
#include "steam/steam_api.h"
#include <iostream>

// ---------------------------------------------------------------------------
// Impl — holds Steamworks callback objects (must live here, not in the header,
// because STEAM_CALLBACK macros expand to member functions that need SDK types)
// ---------------------------------------------------------------------------
struct SteamManager::Impl {
    // Pointer back to owner's m_overlayActive flag
    bool* pOverlayActive = nullptr;

    // Cached leaderboard handles (0 = not yet fetched)
    SteamLeaderboard_t hNormal  = 0;
    SteamLeaderboard_t hEndless = 0;

    // Pending scores waiting for a leaderboard handle
    int  pendingNormal        = 0;
    bool hasPendingNormal     = false;
    int  pendingEndless       = 0;
    bool hasPendingEndless    = false;

    explicit Impl(bool* oa) : pOverlayActive(oa) {}

    // -----------------------------------------------------------------------
    // Steam Overlay — pause game when overlay opens
    // -----------------------------------------------------------------------
    STEAM_CALLBACK(Impl, OnOverlayActivated, GameOverlayActivated_t) {
        if (pOverlayActive)
            *pOverlayActive = (pParam->m_bActive != 0);
    }

    // -----------------------------------------------------------------------
    // Leaderboard: Normal run
    // -----------------------------------------------------------------------
    CCallResult<Impl, LeaderboardFindResult_t>    m_findNormal;
    CCallResult<Impl, LeaderboardScoreUploaded_t> m_uploadNormal;

    void findNormal(int score) {
        pendingNormal    = score;
        hasPendingNormal = true;
        if (hNormal != 0) {
            // Handle already cached — upload immediately
            uploadNormal();
            return;
        }
        SteamAPICall_t h = SteamUserStats()->FindLeaderboard("Dead_Sector_Normal");
        m_findNormal.Set(h, this, &Impl::OnNormalFound);
    }

    void uploadNormal() {
        SteamAPICall_t h = SteamUserStats()->UploadLeaderboardScore(
            hNormal, k_ELeaderboardUploadScoreMethodKeepBest,
            pendingNormal, nullptr, 0);
        m_uploadNormal.Set(h, this, &Impl::OnNormalUploaded);
        hasPendingNormal = false;
    }

    void OnNormalFound(LeaderboardFindResult_t* p, bool ioFail) {
        if (ioFail || !p->m_bLeaderboardFound) {
            std::cerr << "[Steam] Dead_Sector_Normal leaderboard not found\n";
            hasPendingNormal = false;
            return;
        }
        hNormal = p->m_hSteamLeaderboard;
        if (hasPendingNormal) uploadNormal();
    }

    void OnNormalUploaded(LeaderboardScoreUploaded_t* p, bool ioFail) {
        if (!ioFail && p->m_bSuccess)
            std::cerr << "[Steam] Normal score uploaded. New best: "
                      << p->m_bScoreChanged << "\n";
    }

    // -----------------------------------------------------------------------
    // Leaderboard: Endless run
    // -----------------------------------------------------------------------
    CCallResult<Impl, LeaderboardFindResult_t>    m_findEndless;
    CCallResult<Impl, LeaderboardScoreUploaded_t> m_uploadEndless;

    void findEndless(int score) {
        pendingEndless    = score;
        hasPendingEndless = true;
        if (hEndless != 0) {
            uploadEndless();
            return;
        }
        SteamAPICall_t h = SteamUserStats()->FindLeaderboard("Dead_Sector_Endless");
        m_findEndless.Set(h, this, &Impl::OnEndlessFound);
    }

    void uploadEndless() {
        SteamAPICall_t h = SteamUserStats()->UploadLeaderboardScore(
            hEndless, k_ELeaderboardUploadScoreMethodKeepBest,
            pendingEndless, nullptr, 0);
        m_uploadEndless.Set(h, this, &Impl::OnEndlessUploaded);
        hasPendingEndless = false;
    }

    void OnEndlessFound(LeaderboardFindResult_t* p, bool ioFail) {
        if (ioFail || !p->m_bLeaderboardFound) {
            std::cerr << "[Steam] Dead_Sector_Endless leaderboard not found\n";
            hasPendingEndless = false;
            return;
        }
        hEndless = p->m_hSteamLeaderboard;
        if (hasPendingEndless) uploadEndless();
    }

    void OnEndlessUploaded(LeaderboardScoreUploaded_t* p, bool ioFail) {
        if (!ioFail && p->m_bSuccess)
            std::cerr << "[Steam] Endless score uploaded. New best: "
                      << p->m_bScoreChanged << "\n";
    }
};

// ---------------------------------------------------------------------------
// SteamManager methods
// ---------------------------------------------------------------------------

bool SteamManager::init() {
    if (!SteamAPI_Init()) {
        std::cerr << "[Steam] SteamAPI_Init failed — Steam not running or wrong AppID\n";
        m_available = false;
        return false;
    }
    m_available = true;
    std::cerr << "[Steam] Initialised. AppID: " << SteamUtils()->GetAppID() << "\n";

    m_impl = new Impl(&m_overlayActive);

    // Request current stats/achievements so we can query/update them
    SteamUserStats()->RequestCurrentStats();

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
    delete m_impl;
    m_impl = nullptr;
}

void SteamManager::unlockAchievement(const char* apiName) {
    if (!m_available || !apiName) return;
    SteamUserStats()->SetAchievement(apiName);
    SteamUserStats()->StoreStats();
}

void SteamManager::checkCompletionist() {
    if (!m_available) return;
    static const char* PREREQS[] = {
        ACH_FIRST_KILL,     ACH_FIRST_BOSS,    ACH_CLEAR_WORLD1,  ACH_CLEAR_WORLD2,
        ACH_FINAL_BREACH,   ACH_GOLDEN_HULL,   ACH_HIGH_TRACE,
        ACH_KILL_MANTICORE, ACH_KILL_ARCHON,   ACH_KILL_VORTEX,
        ACH_CHAIN_KILL,     ACH_RICOCHET_KILL, ACH_STEALTH_KILL,  ACH_EMP_MULTI,
        ACH_SPEEDRUN,       ACH_ALL_PROGRAMS,  ACH_ALL_HULLS,     ACH_ALL_GOLDEN,
        ACH_LEGENDARY_MOD,  ACH_ENDLESS_10,    ACH_ENDLESS_25,
    };
    for (auto ach : PREREQS) {
        bool achieved = false;
        SteamUserStats()->GetAchievement(ach, &achieved);
        if (!achieved) return;
    }
    unlockAchievement(ACH_FULL_BREACH);
}

void SteamManager::submitNormalScore(int score) {
    if (!m_available || !m_impl) return;
    m_impl->findNormal(score);
}

void SteamManager::submitEndlessScore(int score, int /*wave*/) {
    if (!m_available || !m_impl) return;
    m_impl->findEndless(score);
}

static constexpr const char* CLOUD_SAVE_FILENAME = "dead-sector.sav";

void SteamManager::cloudSave(const std::string& contents) {
    if (!m_available) return;
    bool ok = SteamRemoteStorage()->FileWrite(
        CLOUD_SAVE_FILENAME,
        contents.data(),
        (int32)contents.size());
    if (!ok)
        std::cerr << "[Steam] Cloud save write failed\n";
}

bool SteamManager::cloudLoad(std::string& out) {
    if (!m_available) return false;
    if (!SteamRemoteStorage()->FileExists(CLOUD_SAVE_FILENAME)) return false;
    int32 size = SteamRemoteStorage()->GetFileSize(CLOUD_SAVE_FILENAME);
    if (size <= 0) return false;
    out.resize((size_t)size);
    int32 read = SteamRemoteStorage()->FileRead(
        CLOUD_SAVE_FILENAME,
        &out[0],
        size);
    if (read != size) {
        std::cerr << "[Steam] Cloud save read incomplete (" << read << "/" << size << ")\n";
        out.clear();
        return false;
    }
    return true;
}

#else
// ---------------------------------------------------------------------------
// Stub build — no Steamworks SDK required
// ---------------------------------------------------------------------------

bool SteamManager::init()                           { return false; }
void SteamManager::tick()                           {}
void SteamManager::shutdown()                       {}
void SteamManager::unlockAchievement(const char*)   {}
void SteamManager::checkCompletionist()             {}
void SteamManager::submitNormalScore(int)           {}
void SteamManager::submitEndlessScore(int, int)     {}
void SteamManager::cloudSave(const std::string&)    {}
bool SteamManager::cloudLoad(std::string&)          { return false; }

#endif  // STEAM_ENABLED
