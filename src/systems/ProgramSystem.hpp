#pragma once
#include "core/Programs.hpp"
#include <array>
#include <vector>

class ProgramSystem {
public:
    static constexpr int MAX_SLOTS = 3;
    static constexpr int SLOTS     = MAX_SLOTS;  // HUD compat alias

    // Run-persistent ownership
    bool add(ProgramID id);           // false if full or already owned
    bool has(ProgramID id) const;
    int  count() const { return (int)m_programs.size(); }
    bool full()  const { return count() >= MAX_SLOTS; }
    void reset();                     // full run reset (new game)

    const std::vector<ProgramID>& all() const { return m_programs; }

    // Per-node: reset cooldown timers so all slots start ready
    void resetCooldowns();

    void setCooldownMultiplier(float mult);
    void update(float dt);

    // Slot access — valid for slot 0..SLOTS-1; empty slots return NONE/false
    bool      isEmpty(int slot)       const { return slot >= count(); }
    ProgramID slotID(int slot)        const;
    bool      onCooldown(int slot)    const;
    float     cooldownRatio(int slot) const;
    bool      tryActivate(int slot);

private:
    std::vector<ProgramID>        m_programs;
    std::array<float, MAX_SLOTS>  m_cd     = {};
    float                         m_cdMult = 1.f;
};
