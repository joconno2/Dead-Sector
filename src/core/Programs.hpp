#pragma once
#include <array>
#include <cstdint>
#include <cstdint>

enum class ProgramRarity : uint8_t { Common, Uncommon, Rare };

// Ability type — drives card color theme, independent of rarity
enum class AbilityType : uint8_t {
    Offense,  // bullets / damage     — yellow-orange
    Defense,  // vitality / shields   — green
    Neural,   // programs / cooldowns — purple
    Stealth,  // movement / evasion   — blue
};

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
    AbilityType    abilityType;
};

inline constexpr std::array<ProgramDef, 8> PROGRAM_DEFS = {{
    { ProgramID::FRAG,      "FRAG",      "3-way burst shot",       4.f,  ProgramRarity::Common,   AbilityType::Offense },
    { ProgramID::EMP,       "EMP",       "Stun all ICE for 2s",   12.f,  ProgramRarity::Uncommon, AbilityType::Neural  },
    { ProgramID::STEALTH,   "STEALTH",   "Halt trace for 8s",     15.f,  ProgramRarity::Rare,     AbilityType::Stealth },
    { ProgramID::SHIELD,    "SHIELD",    "Invincible for 2s",     10.f,  ProgramRarity::Common,   AbilityType::Defense },
    { ProgramID::OVERDRIVE, "OVRDRIVE",  "Speed boost for 5s",     8.f,  ProgramRarity::Uncommon, AbilityType::Stealth },
    { ProgramID::DECRYPT,   "DECRYPT",   "Delete nearest ICE",     6.f,  ProgramRarity::Rare,     AbilityType::Neural  },
    { ProgramID::FEEDBACK,  "FEEDBACK",  "Radial burst r=150",     7.f,  ProgramRarity::Rare,     AbilityType::Offense },
    { ProgramID::NONE,      "------",    "[empty slot]",           0.f,  ProgramRarity::Common,   AbilityType::Neural  },
}};

inline const ProgramDef& getProgramDef(ProgramID id) {
    if (id == ProgramID::NONE || static_cast<int>(id) >= static_cast<int>(ProgramID::COUNT))
        return PROGRAM_DEFS[static_cast<int>(ProgramID::COUNT)];
    return PROGRAM_DEFS[static_cast<int>(id)];
}

// Returns the RGB theme color for an ability type
inline void abilityTypeColor(AbilityType t, uint8_t& R, uint8_t& G, uint8_t& B) {
    switch (t) {
        case AbilityType::Offense: R=230; G=160; B=20;  return;  // warm yellow-orange
        case AbilityType::Defense: R=40;  G=210; B=100; return;  // bright green
        case AbilityType::Neural:  R=170; G=50;  B=255; return;  // vivid purple
        case AbilityType::Stealth: R=50;  G=160; B=255; return;  // electric blue
    }
    R=G=B=140;
}

// Returns AbilityType for a given rarity name string for display
inline const char* abilityTypeName(AbilityType t) {
    switch (t) {
        case AbilityType::Offense: return "OFFENSE";
        case AbilityType::Defense: return "DEFENSE";
        case AbilityType::Neural:  return "NEURAL";
        case AbilityType::Stealth: return "STEALTH";
    }
    return "";
}
