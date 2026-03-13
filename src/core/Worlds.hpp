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
            { 20,  70, 115,    0, 210, 245,   255,  70, 140 },
            0, 1.0f
        },
        {
            "DEEP NET", "HOSTILE TERRITORY",
            "Adaptive ICE. Signal has been localized.",
            { 55,  15, 110,  200,  70, 255,   215,  50, 255 },
            1, 1.4f
        },
        {
            "THE CORE", "POINT OF NO RETURN",
            "Maximum ICE density. No retreat option.",
            { 115,  15,  15,  255,  95,  35,   255, 210,  35 },
            2, 1.8f
        },
    };
    return WORLDS[id];
}

inline int worldCount() { return 3; }
