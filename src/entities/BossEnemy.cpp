#include "BossEnemy.hpp"
#include "renderer/VectorRenderer.hpp"
#include "core/Constants.hpp"
#include <cmath>
#include <random>

static constexpr float PI  = 3.14159265f;
static constexpr float TAU = 6.28318530f;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

BossEnemy::BossEnemy(BossType type, Vec2 spawnPos)
    : bossType(type)
{
    pos    = spawnPos;
    vel    = {0.f, 0.f};
    alive  = true;
    noWrap = true;

    switch (type) {
        case BossType::Manticore: hp = maxHp = 12; radius = 45.f; break;
        case BossType::Archon:    hp = maxHp =  8; radius = 40.f; break;
        case BossType::Vortex:    hp = maxHp = 12; radius = 30.f; break;
    }
    m_orbitAngle = -PI * 0.5f;  // start at top
}

const char* BossEnemy::bossName() const {
    switch (bossType) {
        case BossType::Manticore: return "MANTICORE";
        case BossType::Archon:    return "ARCHON";
        case BossType::Vortex:    return "VORTEX";
    }
    return "BOSS";
}

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

Vec2 BossEnemy::shieldPos(int i) const {
    float a = m_spinAngle + i * (TAU / 4.f);
    return pos + Vec2{ std::cos(a) * 68.f, std::sin(a) * 68.f };
}

Vec2 BossEnemy::headPos(int i) const {
    const float angles[4] = { -PI*0.5f, 0.f, PI*0.5f, PI };
    float a = angles[i];
    return pos + Vec2{ std::cos(a) * 55.f, std::sin(a) * 55.f };
}

// ---------------------------------------------------------------------------
// Shield / core hit checks
// ---------------------------------------------------------------------------

bool BossEnemy::tryHitShield(Vec2 projPos, float projRadius) {
    if (bossType != BossType::Archon) return false;
    for (int i = 0; i < 4; ++i) {
        if (!m_shields[i]) continue;
        if ((projPos - shieldPos(i)).length() < projRadius + 14.f) {
            m_shields[i] = false;
            return true;
        }
    }
    return false;
}

bool BossEnemy::coreExposed() const {
    if (bossType == BossType::Archon) {
        for (bool s : m_shields) if (s) return false;
        return true;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

BossEnemy::UpdateResult BossEnemy::update(float dt, Vec2 playerPos) {
    UpdateResult out;
    m_spinAngle += dt * 1.8f;
    if (m_spinAngle > TAU) m_spinAngle -= TAU;

    // Latch phase 2 and signal caller exactly once
    if (!m_phase2 && hp <= maxHp / 2) {
        m_phase2 = true;
        out.phaseTransitioned = true;
        m_fireTimer = 0.f;  // reset fire timer so phase 2 pattern starts fresh
    }

    switch (bossType) {
        case BossType::Manticore: updateManticore(dt, playerPos, out); break;
        case BossType::Archon:    updateArchon   (dt, playerPos, out); break;
        case BossType::Vortex:    updateVortex   (dt, playerPos, out); break;
    }
    return out;
}

static std::unique_ptr<EnemyProjectile> makeProj(Vec2 from, Vec2 dir, float speed = 260.f) {
    auto ep = std::make_unique<EnemyProjectile>(from, dir.normalized() * speed);
    ep->lifetime = 2.2f;
    return ep;
}

void BossEnemy::updateManticore(float dt, Vec2 playerPos, UpdateResult& out) {
    float orbitSpeed = m_phase2 ? 1.4f : 0.85f;
    m_orbitAngle += dt * orbitSpeed;

    const Vec2 centre = { Constants::SCREEN_WF * 0.5f, Constants::SCREEN_HF * 0.5f };
    pos = centre + Vec2{ std::cos(m_orbitAngle) * 185.f, std::sin(m_orbitAngle) * 165.f };

    // Aimed fan attack
    float fireInterval = m_phase2 ? 1.4f : 2.4f;
    m_fireTimer += dt;
    if (m_fireTimer >= fireInterval) {
        m_fireTimer = 0.f;
        Vec2 dir = (playerPos - pos).normalized();
        int spread = m_phase2 ? 7 : 5;
        float step  = m_phase2 ? 0.22f : 0.28f;
        for (int k = 0; k < spread; ++k) {
            float a = std::atan2(dir.y, dir.x) + step * (k - spread / 2);
            out.fired.push_back(makeProj(pos, Vec2::fromAngle(a), 280.f));
        }
    }

    // Phase 2 only: 12-way ring burst every 3.5s
    if (m_phase2) {
        m_burstTimer += dt;
        if (m_burstTimer >= 3.5f) {
            m_burstTimer = 0.f;
            constexpr int RAYS = 12;
            for (int k = 0; k < RAYS; ++k) {
                float a = m_spinAngle + k * (TAU / RAYS);
                out.fired.push_back(makeProj(pos, Vec2::fromAngle(a), 240.f));
            }
        }
    }
}

void BossEnemy::updateArchon(float dt, Vec2 playerPos, UpdateResult& out) {
    bool exposed = coreExposed();
    float orbitSpeed = exposed ? 1.1f : 0.55f;
    m_orbitAngle += dt * orbitSpeed;

    const Vec2 centre = { Constants::SCREEN_WF * 0.5f, Constants::SCREEN_HF * 0.5f };
    pos = centre + Vec2{ std::cos(m_orbitAngle) * 145.f, std::sin(m_orbitAngle) * 130.f };

    float fireInterval = exposed ? 1.6f : 3.0f;
    m_fireTimer += dt;
    if (m_fireTimer >= fireInterval) {
        m_fireTimer = 0.f;
        Vec2 dir = (playerPos - pos).normalized();
        if (exposed) {
            // 3-way spread when shields are down
            for (int k = -1; k <= 1; ++k) {
                float a = std::atan2(dir.y, dir.x) + k * 0.32f;
                out.fired.push_back(makeProj(pos, Vec2::fromAngle(a), 230.f));
            }
        } else {
            out.fired.push_back(makeProj(pos, dir, 200.f));
        }
    }

    // Phase 2: once all shields are destroyed, start a regen countdown (6s → restore one)
    if (m_phase2 && exposed) {
        if (m_shieldRegenTimer < 0.f) {
            m_shieldRegenTimer = 6.f;   // start countdown
        } else {
            m_shieldRegenTimer -= dt;
            if (m_shieldRegenTimer <= 0.f) {
                m_shieldRegenTimer = -1.f;
                // Restore the first destroyed shield
                for (int i = 0; i < 4; ++i) {
                    if (!m_shields[i]) { m_shields[i] = true; break; }
                }
            }
        }
    } else {
        m_shieldRegenTimer = -1.f;  // reset if shields are back up
    }
}

void BossEnemy::updateVortex(float dt, Vec2 playerPos, UpdateResult& out) {
    // Slow drift orbit
    m_orbitAngle += dt * 0.28f;
    const Vec2 centre = { Constants::SCREEN_WF * 0.5f, Constants::SCREEN_HF * 0.5f };
    pos = centre + Vec2{ std::cos(m_orbitAngle) * 190.f, std::sin(m_orbitAngle) * 170.f };

    float headInterval = m_phase2 ? 1.8f : 3.0f;
    for (int i = 0; i < 4; ++i) {
        if (!m_heads[i]) continue;
        m_headFire[i] += dt;
        if (m_headFire[i] >= headInterval) {
            m_headFire[i] = 0.f;
            Vec2 hp2  = headPos(i);
            Vec2 dir  = (playerPos - hp2).normalized();
            out.fired.push_back(makeProj(hp2, dir, 240.f));
            if (m_phase2) {
                // Phase 2: perpendicular shot + 6-way radial burst from body core
                Vec2 perp = { -dir.y, dir.x };
                out.fired.push_back(makeProj(hp2, perp, 210.f));
                // 6-way star burst from the body each time a head fires
                constexpr int SPOKES = 6;
                for (int k = 0; k < SPOKES; ++k) {
                    float a = m_spinAngle + k * (TAU / SPOKES);
                    out.fired.push_back(makeProj(pos, Vec2::fromAngle(a), 200.f));
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void BossEnemy::render(VectorRenderer* vr, float time) const {
    switch (bossType) {
        case BossType::Manticore: renderManticore(vr, time); break;
        case BossType::Archon:    renderArchon   (vr, time); break;
        case BossType::Vortex:    renderVortex   (vr, time); break;
    }
}

void BossEnemy::renderManticore(VectorRenderer* vr, float time) const {
    bool enraged = hp <= maxHp / 2;
    GlowColor col  = enraged ? GlowColor{255, 220, 50} : GlowColor{255, 60, 30};
    GlowColor dim  = enraged ? GlowColor{180, 140, 20} : GlowColor{140, 30, 10};

    // Octagon body
    constexpr int SIDES = 8;
    for (int i = 0; i < SIDES; ++i) {
        float a0 = m_spinAngle * 0.3f + i       * TAU / SIDES;
        float a1 = m_spinAngle * 0.3f + (i + 1) * TAU / SIDES;
        Vec2 p0 = pos + Vec2{ std::cos(a0) * radius, std::sin(a0) * radius };
        Vec2 p1 = pos + Vec2{ std::cos(a1) * radius, std::sin(a1) * radius };
        vr->drawGlowLine(p0, p1, col);
    }
    // Inner ring
    for (int i = 0; i < SIDES; ++i) {
        float a0 = m_spinAngle * 0.3f + i       * TAU / SIDES;
        float a1 = m_spinAngle * 0.3f + (i + 1) * TAU / SIDES;
        Vec2 p0 = pos + Vec2{ std::cos(a0) * 26.f, std::sin(a0) * 26.f };
        Vec2 p1 = pos + Vec2{ std::cos(a1) * 26.f, std::sin(a1) * 26.f };
        vr->drawGlowLine(p0, p1, dim);
    }
    // 4 rotating blades
    for (int i = 0; i < 4; ++i) {
        float a = m_spinAngle + i * (TAU / 4.f);
        Vec2 inner = pos + Vec2{ std::cos(a) * 22.f, std::sin(a) * 22.f };
        Vec2 outer = pos + Vec2{ std::cos(a) * 60.f, std::sin(a) * 60.f };
        Vec2 tip1  = pos + Vec2{ std::cos(a + 0.35f) * 50.f, std::sin(a + 0.35f) * 50.f };
        Vec2 tip2  = pos + Vec2{ std::cos(a - 0.35f) * 50.f, std::sin(a - 0.35f) * 50.f };
        vr->drawGlowLine(inner, outer, col);
        vr->drawGlowLine(outer, tip1, col);
        vr->drawGlowLine(outer, tip2, col);
    }
    // Pulsing core dot
    float pulse = 0.5f + 0.5f * std::sin(time * 6.f);
    float cr = 8.f + 4.f * pulse;
    GlowColor core = {255, 255, (uint8_t)(100 * pulse)};
    for (int i = 0; i < 6; ++i) {
        float a0 = i * TAU / 6.f, a1 = (i + 1) * TAU / 6.f;
        Vec2 p0 = pos + Vec2{ std::cos(a0) * cr, std::sin(a0) * cr };
        Vec2 p1 = pos + Vec2{ std::cos(a1) * cr, std::sin(a1) * cr };
        vr->drawGlowLine(p0, p1, core);
    }
}

void BossEnemy::renderArchon(VectorRenderer* vr, float time) const {
    bool exposed = coreExposed();
    GlowColor col  = exposed ? GlowColor{255, 130, 60} : GlowColor{60, 160, 255};
    GlowColor shCol = GlowColor{200, 230, 255};

    // Outer diamond
    Vec2 pts[4] = {
        pos + Vec2{0, -radius},
        pos + Vec2{radius * 0.75f, 0},
        pos + Vec2{0,  radius},
        pos + Vec2{-radius * 0.75f, 0}
    };
    for (int i = 0; i < 4; ++i)
        vr->drawGlowLine(pts[i], pts[(i + 1) % 4], col);

    // Inner 5-point star
    float starR = 22.f, innerR = 10.f;
    float rot = m_spinAngle * 0.5f;
    for (int i = 0; i < 5; ++i) {
        float a0 = rot + i * TAU / 5.f - PI * 0.5f;
        float a1 = rot + (i + 0.5f) * TAU / 5.f - PI * 0.5f;
        float a2 = rot + (i + 1.f)  * TAU / 5.f - PI * 0.5f;
        Vec2 outer = pos + Vec2{ std::cos(a0) * starR,  std::sin(a0) * starR  };
        Vec2 inner = pos + Vec2{ std::cos(a1) * innerR, std::sin(a1) * innerR };
        Vec2 next  = pos + Vec2{ std::cos(a2) * starR,  std::sin(a2) * starR  };
        vr->drawGlowLine(outer, inner, col);
        vr->drawGlowLine(inner, next,  col);
    }

    // Orbiting shield diamonds
    for (int i = 0; i < 4; ++i) {
        if (!m_shields[i]) continue;
        Vec2 sp = shieldPos(i);
        float pulse = 0.5f + 0.5f * std::sin(time * 4.f + i * PI * 0.5f);
        uint8_t sa = (uint8_t)(180 + 60 * pulse);
        GlowColor sc = {shCol.r, shCol.g, shCol.b};
        (void)sa;
        float s = 11.f;
        vr->drawGlowLine(sp + Vec2{0,-s}, sp + Vec2{ s, 0}, sc);
        vr->drawGlowLine(sp + Vec2{ s, 0}, sp + Vec2{0, s}, sc);
        vr->drawGlowLine(sp + Vec2{0, s}, sp + Vec2{-s, 0}, sc);
        vr->drawGlowLine(sp + Vec2{-s, 0}, sp + Vec2{0,-s}, sc);
    }

    if (exposed) {
        // Exposed core — pulsing warning glow
        float pulse = 0.5f + 0.5f * std::sin(time * 8.f);
        GlowColor warn = {255, (uint8_t)(80 + 120 * pulse), 20};
        for (int i = 0; i < 4; ++i) {
            float a0 = i * TAU / 4.f, a1 = (i + 1) * TAU / 4.f;
            float cr = 14.f + 4.f * pulse;
            Vec2 p0 = pos + Vec2{std::cos(a0)*cr, std::sin(a0)*cr};
            Vec2 p1 = pos + Vec2{std::cos(a1)*cr, std::sin(a1)*cr};
            vr->drawGlowLine(p0, p1, warn);
        }
    }
}

void BossEnemy::renderVortex(VectorRenderer* vr, float time) const {
    bool enraged = hp <= maxHp / 2;
    GlowColor col  = enraged ? GlowColor{180, 255, 60} : GlowColor{60, 220, 80};
    GlowColor hCol = enraged ? GlowColor{230, 255, 100} : GlowColor{120, 255, 120};

    // Hexagon body
    constexpr int HEX = 6;
    for (int i = 0; i < HEX; ++i) {
        float a0 = m_spinAngle * 0.4f + i       * TAU / HEX;
        float a1 = m_spinAngle * 0.4f + (i + 1) * TAU / HEX;
        Vec2 p0 = pos + Vec2{std::cos(a0) * radius, std::sin(a0) * radius};
        Vec2 p1 = pos + Vec2{std::cos(a1) * radius, std::sin(a1) * radius};
        vr->drawGlowLine(p0, p1, col);
    }
    // Inner hexagon
    for (int i = 0; i < HEX; ++i) {
        float a0 = m_spinAngle * 0.4f + i       * TAU / HEX;
        float a1 = m_spinAngle * 0.4f + (i + 1) * TAU / HEX;
        Vec2 p0 = pos + Vec2{std::cos(a0) * 16.f, std::sin(a0) * 16.f};
        Vec2 p1 = pos + Vec2{std::cos(a1) * 16.f, std::sin(a1) * 16.f};
        vr->drawGlowLine(p0, p1, col);
    }

    // 4 arm heads (cardinal)
    const float angles[4] = { -PI*0.5f, 0.f, PI*0.5f, PI };
    for (int i = 0; i < 4; ++i) {
        if (!m_heads[i]) continue;
        Vec2 hp2  = headPos(i);
        float ha  = angles[i];
        float pulse = 0.5f + 0.5f * std::sin(time * 5.f + i * PI * 0.5f);

        // Arm shaft from body edge to head
        Vec2 arm0 = pos  + Vec2{std::cos(ha) * radius, std::sin(ha) * radius};
        Vec2 arm1 = hp2;
        vr->drawGlowLine(arm0, arm1, col);

        // Head: fork shape (2 claws)
        float cl = 12.f + 3.f * pulse;
        float fw  = 0.5f;
        Vec2 claw1 = hp2 + Vec2{std::cos(ha + fw) * cl, std::sin(ha + fw) * cl};
        Vec2 claw2 = hp2 + Vec2{std::cos(ha - fw) * cl, std::sin(ha - fw) * cl};
        Vec2 tip   = hp2 + Vec2{std::cos(ha) * cl * 1.3f, std::sin(ha) * cl * 1.3f};
        vr->drawGlowLine(hp2, claw1, hCol);
        vr->drawGlowLine(hp2, claw2, hCol);
        vr->drawGlowLine(hp2, tip,   hCol);
        // Small diamond at head base
        float ds = 7.f;
        Vec2 perp = {-std::sin(ha), std::cos(ha)};
        vr->drawGlowLine(hp2 - perp*ds, hp2 + Vec2{std::cos(ha)*ds, std::sin(ha)*ds}, hCol);
        vr->drawGlowLine(hp2 + perp*ds, hp2 + Vec2{std::cos(ha)*ds, std::sin(ha)*ds}, hCol);
    }
}
