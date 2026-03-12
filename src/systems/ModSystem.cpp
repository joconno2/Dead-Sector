#include "ModSystem.hpp"
#include <algorithm>
#include <random>

// ---------------------------------------------------------------------------
// Mutation
// ---------------------------------------------------------------------------

void ModSystem::add(ModID id) {
    m_mods.push_back(id);
}

bool ModSystem::has(ModID id) const {
    return std::any_of(m_mods.begin(), m_mods.end(),
                       [id](ModID m){ return m == id; });
}

int ModSystem::count(ModID id) const {
    return static_cast<int>(std::count(m_mods.begin(), m_mods.end(), id));
}

void ModSystem::reset() {
    m_mods.clear();
}

// ---------------------------------------------------------------------------
// Stat multipliers
// ---------------------------------------------------------------------------

float ModSystem::overclockMult() const {
    return has(ModID::NEURAL_OVERCLOCK) ? 1.20f : 1.f;
}

float ModSystem::thrustMult() const {
    float v = 1.f;
    for (int i = 0; i < count(ModID::KINETIC_CORE); ++i) v *= 1.4f;
    return v * overclockMult();
}

float ModSystem::rotMult() const {
    float v = 1.f;
    for (int i = 0; i < count(ModID::GYRO_STAB); ++i) v *= 1.4f;
    return v * overclockMult();
}

float ModSystem::maxSpeedMult() const {
    float v = 1.f;
    for (int i = 0; i < count(ModID::INERTIA_DAMP); ++i) v *= 1.25f;
    return v * overclockMult();
}

float ModSystem::projSpeedMult() const {
    float v = 1.f;
    for (int i = 0; i < count(ModID::HOT_BARREL); ++i) v *= 1.30f;
    return v * overclockMult();
}

float ModSystem::projRadiusMult() const {
    float v = has(ModID::WIDE_BEAM) ? 2.0f : 1.f;
    return v * overclockMult();
}

float ModSystem::cdMult() const {
    // Cooldown mult reduces duration — overclock doesn't stack here (intentional balance)
    float v = 1.f;
    for (int i = 0; i < count(ModID::COLD_EXEC); ++i) v *= 0.75f;
    return v;
}

float ModSystem::traceSinkAmt() const {
    return count(ModID::TRACE_SINK) * 6.f;
}

int ModSystem::startingExtraLives() const {
    int lives = 0;
    if (has(ModID::ADAPTIVE_ARMOR)) lives += 1;
    lives += count(ModID::HULL_PLATING);
    return lives;
}

// ---------------------------------------------------------------------------
// Passive slot tracking
// ---------------------------------------------------------------------------

int ModSystem::passiveCount() const {
    int n = 0;
    for (ModID id : m_mods)
        if (getModDef(id).type == ModType::Passive) ++n;
    return n;
}

// ---------------------------------------------------------------------------
// Weighted offer pool
// ---------------------------------------------------------------------------

// Rarity weights: Common=50, Uncommon=20, Rare=7, Legendary=2
static int rarityWeight(ModRarity r) {
    switch (r) {
        case ModRarity::Common:    return 50;
        case ModRarity::Uncommon:  return 20;
        case ModRarity::Rare:      return 7;
        case ModRarity::Legendary: return 2;
    }
    return 1;
}

std::vector<ModID> ModSystem::buildOfferPool(int n, bool passiveSlotsAvail) const {
    struct Candidate { ModID id; int weight; };
    std::vector<Candidate> candidates;
    candidates.reserve(static_cast<int>(ModID::COUNT));

    const int total = static_cast<int>(ModID::COUNT);
    for (int i = 0; i < total; ++i) {
        ModID id = static_cast<ModID>(i);
        const ModDef& def = getModDef(id);
        // Skip Passive mods when passive slots are full
        if (def.type == ModType::Passive && !passiveSlotsAvail) continue;
        // Skip non-stackable mods already owned
        if (!def.stackable && has(id)) continue;
        candidates.push_back({ id, rarityWeight(def.rarity) });
    }

    std::mt19937 rng(std::random_device{}());
    std::vector<ModID> result;
    result.reserve(n);

    // Weighted sampling without replacement
    while ((int)result.size() < n && !candidates.empty()) {
        int totalWeight = 0;
        for (auto& c : candidates) totalWeight += c.weight;

        std::uniform_int_distribution<int> dist(0, totalWeight - 1);
        int roll = dist(rng);

        int acc = 0, chosen = 0;
        for (int j = 0; j < (int)candidates.size(); ++j) {
            acc += candidates[j].weight;
            if (roll < acc) { chosen = j; break; }
        }

        result.push_back(candidates[chosen].id);
        candidates.erase(candidates.begin() + chosen);
    }

    return result;
}
