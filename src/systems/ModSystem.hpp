#pragma once
#include "core/Mods.hpp"
#include <vector>

class ModSystem {
public:
    void add(ModID id);
    bool has(ModID id) const;
    int  count(ModID id) const;

    const std::vector<ModID>& all() const { return m_mods; }

    void reset();

    // ---- Stat multipliers (NEURAL_OVERCLOCK chains through all of these) ----
    float thrustMult()    const;   // KINETIC_CORE stacks × overclock
    float rotMult()       const;   // GYRO_STAB stacks × overclock
    float maxSpeedMult()  const;   // INERTIA_DAMP stacks × overclock
    float projSpeedMult() const;   // HOT_BARREL stacks × overclock
    float projRadiusMult()const;   // WIDE_BEAM × overclock
    float cdMult()        const;   // COLD_EXEC stacks (applied to cooldown, NOT overclocked)
    float traceSinkAmt()  const;   // TRACE_SINK: % per kill
    float overclockMult() const;   // NEURAL_OVERCLOCK: 1.20 or 1.0

    // ---- Boolean abilities ----
    bool hasSplitRound()      const { return has(ModID::SPLIT_ROUND);      }
    bool hasAdaptiveArmor()   const { return has(ModID::ADAPTIVE_ARMOR);   }
    bool hasPhaseFrame()      const { return has(ModID::PHASE_FRAME);      }
    bool hasOvercharge()      const { return has(ModID::OVERCHARGE);       }
    bool hasNovaBurst()       const { return has(ModID::NOVA_BURST);       }
    bool hasRicochet()        const { return has(ModID::RICOCHET);         }
    bool hasChainFire()       const { return has(ModID::CHAIN_FIRE);       }
    bool hasReactivePlating() const { return has(ModID::REACTIVE_PLATING); }
    bool hasPhantomRound()    const { return has(ModID::PHANTOM_ROUND);    }
    bool hasCritMatrix()      const { return has(ModID::CRIT_MATRIX);      }
    bool hasGhostProtocol()   const { return has(ModID::GHOST_PROTOCOL);   }

    // Extra lives granted at node start (ADAPTIVE_ARMOR=1, +HULL_PLATING stacks)
    int  startingExtraLives() const;

    // Passive slot tracking (max 3)
    int  passiveCount() const;
    bool passiveFull()  const { return passiveCount() >= 3; }

    // Build a weighted-random offer pool of N mods (respects rarity weights,
    // excludes Passive mods when passive slots are full,
    // excludes non-stackable mods already owned).
    std::vector<ModID> buildOfferPool(int n, bool passiveSlotsAvail) const;

private:
    std::vector<ModID> m_mods;
};
