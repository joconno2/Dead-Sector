#include "ProgramSystem.hpp"
#include <algorithm>

bool ProgramSystem::add(ProgramID id) {
    if (full() || has(id)) return false;
    m_programs.push_back(id);
    return true;
}

bool ProgramSystem::has(ProgramID id) const {
    return std::any_of(m_programs.begin(), m_programs.end(),
                       [id](ProgramID p){ return p == id; });
}

void ProgramSystem::reset() {
    m_programs.clear();
    m_cd     = {};
    m_cdMult = 1.f;
}

void ProgramSystem::resetCooldowns() {
    m_cd = {};
}

void ProgramSystem::setCooldownMultiplier(float mult) {
    m_cdMult = mult;
}

void ProgramSystem::update(float dt) {
    for (auto& cd : m_cd)
        if (cd > 0.f) cd = std::max(0.f, cd - dt);
}

bool ProgramSystem::tryActivate(int slot) {
    if (slot < 0 || slot >= count()) return false;
    if (m_cd[slot] > 0.f) return false;
    m_cd[slot] = getProgramDef(m_programs[slot]).cooldown * m_cdMult;
    return true;
}

ProgramID ProgramSystem::slotID(int slot) const {
    if (slot < 0 || slot >= count()) return ProgramID::NONE;
    return m_programs[slot];
}

bool ProgramSystem::onCooldown(int slot) const {
    if (slot < 0 || slot >= count()) return true;  // empty = unavailable
    return m_cd[slot] > 0.f;
}

float ProgramSystem::cooldownRatio(int slot) const {
    if (slot < 0 || slot >= count()) return 0.f;
    float maxCd = getProgramDef(m_programs[slot]).cooldown * m_cdMult;
    if (maxCd <= 0.f) return 0.f;
    return std::clamp(m_cd[slot] / maxCd, 0.f, 1.f);
}
