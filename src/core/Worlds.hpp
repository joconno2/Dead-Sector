#pragma once
#include <cstdint>

// World IDs: 0=SECTOR ALPHA, 1=DEEP NET, 2=THE CORE
// bossType matches the BossType enum (0=Manticore, 1=Archon, 2=Vortex)

struct WorldTheme {
    uint8_t gridR, gridG, gridB;        // background grid color
    uint8_t accentR, accentG, accentB;  // wall / UI accent color
    uint8_t constructR, constructG, constructB; // DataConstruct tint
};

struct WorldDef {
    const char* name;
    const char* subtitle;
    const char* flavor;
    WorldTheme  theme;
    int         bossType;   // matches BossType enum
    float       diffMult;   // multiplier applied to trace tick rate
};

inline const WorldDef& worldDef(int id) {
    if (id < 0) id = 0;
    if (id > 2) id = 2;
    static const WorldDef WORLDS[3] = {
        {
            "SECTOR ALPHA", "ENTRY POINT",
            "Tier-1 ICE. Standard countermeasures.",
            { 15,  50,  80,    0, 190, 220,   255,  60, 120 },
            0, 1.0f
        },
        {
            "DEEP NET", "HOSTILE TERRITORY",
            "Adaptive ICE. Signal has been localized.",
            { 40,  10,  80,  180,  60, 255,   200,  40, 255 },
            1, 1.4f
        },
        {
            "THE CORE", "POINT OF NO RETURN",
            "Maximum ICE density. No retreat option.",
            { 80,  10,  10,  255,  80,  30,   255, 200,  30 },
            2, 1.8f
        },
    };
    return WORLDS[id];
}

inline int worldCount() { return 3; }
