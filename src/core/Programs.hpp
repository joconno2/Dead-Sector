#pragma once
#include <array>
#include <cstdint>

enum class ProgramRarity : uint8_t { Common, Uncommon, Rare };

enum class ProgramID : uint8_t {
    FRAG      = 0,  // 3-way spread shot
    EMP       = 1,  // Stun all ICE for 2s
    STEALTH   = 2,  // Halt trace growth for 8s
    SHIELD    = 3,  // Invincibility for 2s
    OVERDRIVE = 4,  // Speed boost for 5s
    DECRYPT   = 5,  // Destroy nearest ICE instantly
    FEEDBACK  = 6,  // Radial damage burst (r=150)
    COUNT     = 7,
    NONE      = 0xFF  // empty slot sentinel
};

struct ProgramDef {
    ProgramID      id;
    const char*    name;
    const char*    desc;
    float          cooldown;
    ProgramRarity  rarity;
};

inline constexpr std::array<ProgramDef, 8> PROGRAM_DEFS = {{
    { ProgramID::FRAG,      "FRAG",     "3-way burst shot",       4.f,  ProgramRarity::Common   },
    { ProgramID::EMP,       "EMP",      "Stun all ICE for 2s",   12.f,  ProgramRarity::Uncommon },
    { ProgramID::STEALTH,   "STEALTH",  "Halt trace for 8s",     15.f,  ProgramRarity::Rare     },
    { ProgramID::SHIELD,    "SHIELD",   "Invincible for 2s",     10.f,  ProgramRarity::Common   },
    { ProgramID::OVERDRIVE, "OVRDRIVE", "Speed boost for 5s",     8.f,  ProgramRarity::Uncommon },
    { ProgramID::DECRYPT,   "DECRYPT",  "Delete nearest ICE",     6.f,  ProgramRarity::Rare     },
    { ProgramID::FEEDBACK,  "FEEDBACK", "Radial burst r=150",     7.f,  ProgramRarity::Rare     },
    { ProgramID::NONE,      "------",   "[empty slot]",           0.f,  ProgramRarity::Common   },
}};

inline const ProgramDef& getProgramDef(ProgramID id) {
    if (id == ProgramID::NONE || static_cast<int>(id) >= static_cast<int>(ProgramID::COUNT))
        return PROGRAM_DEFS[static_cast<int>(ProgramID::COUNT)]; // NONE entry
    return PROGRAM_DEFS[static_cast<int>(id)];
}
