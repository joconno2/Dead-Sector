#pragma once
#include <string>
#include <vector>

// Effect applied when an Event node is entered.
// Effects are cumulative with the next CombatScene's starting state.
struct EventDef {
    const char* id;
    const char* title;       // short header, all-caps
    const char* body;        // 2–3 lines of flavor/description
    const char* outcome;     // one-liner summary of what happened
    int   creditDelta;       // added to saveData->credits immediately (can be negative)
    float tracePenalty;      // added to ctx.nextNodeStartTrace (0–100)
    int   livesBonus;        // added to ctx.bonusLives for next combat
};

// clang-format off
static const EventDef EVENT_POOL[] = {
    // ---- BUFFS -------------------------------------------------------
    {
        "DATA_CACHE",
        "DATA CACHE",
        "An unencrypted archive sits on an abandoned subnet.\n"
        "Whoever owned this forgot to wipe it.",
        "ACQUIRED: +13 CREDITS",
        13, 0.f, 0
    },
    {
        "CORP_BACKDOOR",
        "CORPORATE BACKDOOR",
        "A maintenance account never decommissioned.\n"
        "The corp doesn't even know it exists.",
        "CREDIT TRANSFER COMPLETE: +18 CREDITS",
        18, 0.f, 0
    },
    {
        "ICE_CRYPT",
        "ICE CRYPT",
        "A locked sector sealed with decade-old encryption.\n"
        "You crack it in four seconds flat.",
        "CRYPT BREACHED: +9 CREDITS",
        9, 0.f, 0
    },
    {
        "GHOST_MARKET",
        "GHOST MARKET",
        "A shadow-net bazaar node — buyers and sellers, no names.\n"
        "Someone left their credit line open.",
        "GHOST TRANSFER: +15 CREDITS",
        15, 0.f, 0
    },
    {
        "FIRMWARE_UPDATE",
        "FIRMWARE UPDATE",
        "Legacy medical firmware, still broadcasting patches.\n"
        "Your wetware absorbs it greedily.",
        "SYSTEM HARDENED: +1 EXTRA LIFE",
        0, 0.f, 1
    },
    {
        "NEURAL_BOOST",
        "NEURAL BOOST",
        "An experimental kernel fragment — origin unknown.\n"
        "It slots into your deck like it was made for you.",
        "AUGMENTED: +1 EXTRA LIFE  +5 CREDITS",
        5, 0.f, 1
    },
    {
        "KERNEL_EXPLOIT",
        "KERNEL EXPLOIT",
        "A zero-day in the local hypervisor, unpublished.\n"
        "You extract every credit register you can reach.",
        "ROOT ACCESS: +20 CREDITS",
        20, 0.f, 0
    },
    {
        "ABANDONED_TERMINAL",
        "ABANDONED TERMINAL",
        "A node from the old net, before the corps took over.\n"
        "Its memory banks are still full.",
        "ARCHIVES LOOTED: +11 CREDITS",
        11, 0.f, 0
    },
    // ---- DEBUFFS -----------------------------------------------------
    {
        "HONEYPOT",
        "HONEYPOT",
        "The node looked clean. It wasn't.\n"
        "A tracer latches on before you can jack out.\n"
        "ICE is already spinning up in the next sector.",
        "TRACED: NEXT NODE +25% STARTING TRACE",
        0, 25.f, 0
    },
    {
        "NEURAL_FEEDBACK",
        "NEURAL FEEDBACK",
        "A feedback pulse burns through your deck's buffer.\n"
        "Credits scatter across the subnet — unrecoverable.\n"
        "The alarm signal is already propagating.",
        "DAMAGE: -8 CREDITS  NEXT NODE +15% TRACE",
        -8, 15.f, 0
    },
    {
        "SYSTEM_PURGE",
        "SYSTEM PURGE",
        "Automated security kicks in — everything in cache\n"
        "is wiped before you can grab it.\n"
        "You leave with nothing.",
        "PURGED: -13 CREDITS",
        -13, 0.f, 0
    },
    {
        "BLACK_ICE_TRAP",
        "BLACK ICE TRAP",
        "Something was watching. It was waiting for exactly\n"
        "this moment. Black ICE floods the sector.\n"
        "Run.",
        "AMBUSHED: NEXT NODE +30% STARTING TRACE",
        0, 30.f, 0
    },
};
// clang-format on

static constexpr int EVENT_POOL_SIZE = (int)(sizeof(EVENT_POOL) / sizeof(EVENT_POOL[0]));
