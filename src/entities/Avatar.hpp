#pragma once
#include "Entity.hpp"
#include <vector>
#include <string>

class Projectile;

enum class HullType { Delta, Raptor, Mantis, Blade, Battle };

// Per-hull stat multipliers and metadata
struct HullStats {
    float       thrustMult    = 1.f;
    float       speedMult     = 1.f;
    float       rotMult       = 1.f;
    float       projSpeedMult = 1.f;
    float       radiusMult    = 1.f;
    int         startingAmmo  = 30;
    int         extraLives    = 0;
    const char* id            = "DELTA";
    const char* name          = "GHOST RUNNER";
    const char* flavor        = "Standard-issue black-market vessel. No surprises.";
    // Unlock thresholds (0 = always unlocked)
    int         runsRequired  = 0;
    int         killsRequired = 0;
    bool        needsWin      = false;
};

HullStats             statsForHull(HullType hull);
std::vector<Vec2>     hullVerts(HullType hull);   // shared geometry used by render and UI

// Resolve hull type from a save-data string ID
inline HullType hullFromString(const std::string& s) {
    if (s == "RAPTOR")  return HullType::Raptor;
    if (s == "MANTIS")  return HullType::Mantis;
    if (s == "BLADE")   return HullType::Blade;
    if (s == "BATTLE")  return HullType::Battle;
    return HullType::Delta;
}

class Avatar : public Entity {
public:
    Avatar(float startX, float startY, HullType hull = HullType::Delta);

    void update(float dt) override;

    void applyThrust(float dt, float forceMult = 1.f, float speedCapMult = 1.f);
    void rotateLeft(float dt);
    void rotateRight(float dt);

    // Returns a heap-allocated Projectile with optional speed/radius multipliers.
    Projectile* fire(float speedMult = 1.f, float radiusMult = 1.f);

    bool  thrusting     = false;
    float thrusterTimer = 0.f;

    // Program effects
    bool  shielded       = false;
    float shieldTimer    = 0.f;
    float overdriveTimer = 0.f;

    // Mod effects
    int   extraLives     = 0;

    // Hull-specific multipliers (read by CombatScene for thrust/rot/proj)
    HullStats hullStats;
};
