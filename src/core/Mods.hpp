#pragma once
#include <array>
#include <cstdint>

enum class ModRarity   : uint8_t { Common, Uncommon, Rare, Legendary };
enum class ModCategory : uint8_t { Chassis, Weapon, Neural };
// Stat = stackable numeric upgrade (no slot limit)
// Passive = unique ability that occupies one of 3 passive slots
enum class ModType     : uint8_t { Stat, Passive };

enum class ModID : uint8_t {
    // === CHASSIS ===
    KINETIC_CORE,      // Common    - Thrust +40%              (stackable)
    GYRO_STAB,         // Common    - Rotation +40%            (stackable)
    INERTIA_DAMP,      // Common    - Max velocity +25%        (stackable)
    ADAPTIVE_ARMOR,    // Uncommon  - Absorb 1 lethal hit/node (non-stackable)
    HULL_PLATING,      // Rare      - +1 extra life at start   (stackable)
    PHASE_FRAME,       // Rare      - Each kill: 0.5s invuln   (non-stackable)
    REACTIVE_PLATING,  // Rare      - Hit: 150px damage burst  (non-stackable)

    // === WEAPON ===
    HOT_BARREL,        // Common    - Proj speed +30%          (stackable)
    WIDE_BEAM,         // Uncommon  - Hit radius +100%         (non-stackable)
    SPLIT_ROUND,       // Uncommon  - Every 3rd shot fires twin (non-stackable)
    OVERCHARGE,        // Rare      - Every 5th shot: burst×3  (non-stackable)
    NOVA_BURST,        // Rare      - Kill: 4-shot radial nova  (non-stackable)
    RICOCHET,          // Uncommon  - Shots bounce off edges   (non-stackable)
    CHAIN_FIRE,        // Legendary - Kill: instant shot at next ICE (non-stackable)
    PHANTOM_ROUND,     // Legendary - Shots pierce ALL ICE      (non-stackable)
    CRIT_MATRIX,       // Legendary - 30%: shot survives kill  (non-stackable)

    // === NEURAL ===
    COLD_EXEC,         // Common    - Program CDs -25%          (stackable)
    TRACE_SINK,        // Uncommon  - Each kill -6% trace       (stackable)
    GHOST_PROTOCOL,    // Rare      - Trace tick rate -40%      (non-stackable)
    NEURAL_OVERCLOCK,  // Legendary - All stat mults +20%       (non-stackable)

    COUNT
};

struct ModDef {
    ModID        id;
    const char*  name;
    const char*  desc;
    ModCategory  category;
    ModRarity    rarity;
    ModType      type;      // Stat (no slot) vs Passive (1 of 3 slots)
    bool         stackable;
};

inline constexpr std::array<ModDef, static_cast<int>(ModID::COUNT)> MOD_DEFS = {{
    // Chassis — Stat (stackable, no slot cost)
    { ModID::KINETIC_CORE,     "KINETIC CORE",     "Thrust force +40%",               ModCategory::Chassis, ModRarity::Common,    ModType::Stat,    true  },
    { ModID::GYRO_STAB,        "GYRO STAB",        "Rotation speed +40%",             ModCategory::Chassis, ModRarity::Common,    ModType::Stat,    true  },
    { ModID::INERTIA_DAMP,     "INERTIA DAMP",     "Max velocity +25%",               ModCategory::Chassis, ModRarity::Common,    ModType::Stat,    true  },
    // Chassis — Passive
    { ModID::ADAPTIVE_ARMOR,   "ADAPT. ARMOR",     "Absorb 1 lethal hit per node",    ModCategory::Chassis, ModRarity::Uncommon,  ModType::Passive, false },
    { ModID::HULL_PLATING,     "HULL PLATING",     "+1 extra life at node start",     ModCategory::Chassis, ModRarity::Rare,      ModType::Stat,    true  },
    { ModID::PHASE_FRAME,      "PHASE FRAME",      "Each kill: 0.5s invulnerability", ModCategory::Chassis, ModRarity::Rare,      ModType::Passive, false },
    { ModID::REACTIVE_PLATING, "REACT. PLATING",   "Hit: 150px damage shockwave",     ModCategory::Chassis, ModRarity::Rare,      ModType::Passive, false },
    // Weapon — Stat
    { ModID::HOT_BARREL,       "HOT BARREL",       "Projectile speed +30%",           ModCategory::Weapon,  ModRarity::Common,    ModType::Stat,    true  },
    // Weapon — Passive
    { ModID::WIDE_BEAM,        "WIDE BEAM",        "Hit radius +100%  (double)",      ModCategory::Weapon,  ModRarity::Uncommon,  ModType::Passive, false },
    { ModID::SPLIT_ROUND,      "SPLIT ROUND",      "Every 3rd shot fires a twin",     ModCategory::Weapon,  ModRarity::Uncommon,  ModType::Passive, false },
    { ModID::OVERCHARGE,       "OVERCHARGE",       "Every 5th shot: burst of 3",      ModCategory::Weapon,  ModRarity::Rare,      ModType::Passive, false },
    { ModID::NOVA_BURST,       "NOVA BURST",       "Kill: 4-shot cross nova",         ModCategory::Weapon,  ModRarity::Rare,      ModType::Passive, false },
    { ModID::RICOCHET,         "RICOCHET",         "Shots bounce off screen edges",   ModCategory::Weapon,  ModRarity::Uncommon,  ModType::Passive, false },
    { ModID::CHAIN_FIRE,       "CHAIN FIRE",       "Kill: instant shot at next ICE",  ModCategory::Weapon,  ModRarity::Legendary, ModType::Passive, false },
    { ModID::PHANTOM_ROUND,    "PHANTOM ROUND",    "Shots pierce through all ICE",    ModCategory::Weapon,  ModRarity::Legendary, ModType::Passive, false },
    { ModID::CRIT_MATRIX,      "CRIT MATRIX",      "30% chance: shot survives kill",  ModCategory::Weapon,  ModRarity::Legendary, ModType::Passive, false },
    // Neural — Stat
    { ModID::COLD_EXEC,        "COLD EXEC",        "Program cooldowns -25%",          ModCategory::Neural,  ModRarity::Common,    ModType::Stat,    true  },
    { ModID::TRACE_SINK,       "TRACE SINK",       "Each kill reduces trace 6%",      ModCategory::Neural,  ModRarity::Uncommon,  ModType::Stat,    true  },
    // Neural — Passive
    { ModID::GHOST_PROTOCOL,   "GHOST PROTOCOL",   "Trace tick rate -40%",            ModCategory::Neural,  ModRarity::Rare,      ModType::Passive, false },
    { ModID::NEURAL_OVERCLOCK, "NEURAL OVERCLOCK", "All stat multipliers +20%",       ModCategory::Neural,  ModRarity::Legendary, ModType::Passive, false },
}};

inline const ModDef& getModDef(ModID id) {
    return MOD_DEFS[static_cast<int>(id)];
}

// Rarity display names and border colors
inline const char* rarityName(ModRarity r) {
    switch (r) {
        case ModRarity::Common:    return "COMMON";
        case ModRarity::Uncommon:  return "UNCOMMON";
        case ModRarity::Rare:      return "RARE";
        case ModRarity::Legendary: return "LEGENDARY";
    }
    return "";
}

// Returns (R,G,B) highlight color for a rarity tier
inline void rarityColor(ModRarity r, uint8_t& R, uint8_t& G, uint8_t& B) {
    switch (r) {
        case ModRarity::Common:    R=170; G=170; B=170; return;  // silver
        case ModRarity::Uncommon:  R=40;  G=200; B=100; return;  // green
        case ModRarity::Rare:      R=60;  G=140; B=255; return;  // blue
        case ModRarity::Legendary: R=255; G=185; B=20;  return;  // gold
    }
    R=G=B=128;
}
