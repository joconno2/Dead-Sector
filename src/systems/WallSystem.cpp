#include "WallSystem.hpp"
#include "renderer/VectorRenderer.hpp"
#include "core/Constants.hpp"
#include <random>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------

Vec2 WallSystem::closestPointOnSegment(Vec2 p, Vec2 a, Vec2 b) {
    Vec2 ab = b - a;
    Vec2 ap = p - a;
    float lsq = ab.lengthSq();
    if (lsq < 0.0001f) return a;
    float t = std::max(0.f, std::min(1.f, ap.dot(ab) / lsq));
    return a + ab * t;
}

// ---------------------------------------------------------------------------
// Generation
// ---------------------------------------------------------------------------

void WallSystem::generate(int tier, int nodeId) {
    m_walls.clear();

    // Seed with nodeId so walls are deterministic per node
    std::mt19937 rng(static_cast<uint32_t>(nodeId * 3571 + 1237));

    // Counts by tier: 1→2, 2→4, 3→6, 4→8
    int count = tier * 2;

    std::uniform_real_distribution<float> angDist(0.f, 3.14159f); // half-circle avoids duplicates
    std::uniform_real_distribution<float> lenDist(55.f, 180.f + tier * 25.f);

    const float W = Constants::SCREEN_WF;
    const float H = Constants::SCREEN_HF;
    const float CLEAR_R  = 110.f; // keep center clear for spawn
    const float EDGE_PAD = 40.f;

    std::uniform_real_distribution<float> xDist(EDGE_PAD, W - EDGE_PAD);
    std::uniform_real_distribution<float> yDist(EDGE_PAD, H - EDGE_PAD);

    int attempts = 0;
    while ((int)m_walls.size() < count && attempts < count * 12) {
        ++attempts;
        float cx = xDist(rng), cy = yDist(rng);

        // Keep center clear
        float dx = cx - W * 0.5f, dy = cy - H * 0.5f;
        if (std::sqrt(dx*dx + dy*dy) < CLEAR_R) continue;

        float angle = angDist(rng);
        float halfLen = lenDist(rng) * 0.5f;
        float cosA = std::cos(angle), sinA = std::sin(angle);

        Vec2 a = { cx - cosA * halfLen, cy - sinA * halfLen };
        Vec2 b = { cx + cosA * halfLen, cy + sinA * halfLen };

        // Clamp endpoints to screen
        a.x = std::max(EDGE_PAD, std::min(W - EDGE_PAD, a.x));
        a.y = std::max(EDGE_PAD, std::min(H - EDGE_PAD, a.y));
        b.x = std::max(EDGE_PAD, std::min(W - EDGE_PAD, b.x));
        b.y = std::max(EDGE_PAD, std::min(H - EDGE_PAD, b.y));

        // Normal: perpendicular to segment
        Vec2 seg = (b - a).normalized();
        Vec2 norm = { -seg.y, seg.x };

        m_walls.push_back({ a, b, norm });
    }
}

// ---------------------------------------------------------------------------
// Collision resolution
// ---------------------------------------------------------------------------

bool WallSystem::resolveCircle(Vec2& pos, Vec2& vel, float radius, float energyLoss) const {
    bool hit = false;
    // Multiple iterations for stability at corners
    for (int iter = 0; iter < 2; ++iter) {
        for (const auto& w : m_walls) {
            Vec2 closest = closestPointOnSegment(pos, w.a, w.b);
            Vec2 diff    = pos - closest;
            float dist   = diff.length();

            if (dist < radius + 1.f && dist > 0.001f) {
                Vec2 n = diff * (1.f / dist);

                // Push out
                float pen = (radius + 1.f) - dist;
                pos = pos + n * pen;

                // Reflect velocity off wall normal
                float vDotN = vel.dot(n);
                if (vDotN < 0.f) { // only reflect if moving into wall
                    vel = vel - n * (2.f * vDotN);
                    vel = vel * energyLoss;
                }
                hit = true;
            }
        }
    }
    return hit;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void WallSystem::render(VectorRenderer* vr, uint8_t r, uint8_t g, uint8_t b) const {
    GlowColor col  = { r, g, b };
    GlowColor core = { (uint8_t)std::min(255, (int)r + 60),
                       (uint8_t)std::min(255, (int)g + 80),
                       (uint8_t)std::min(255, (int)b + 80) };

    for (const auto& w : m_walls) {
        Vec2 seg  = w.b - w.a;
        float len = seg.length();
        if (len < 0.001f) continue;
        Vec2 dir  = seg * (1.f / len);

        // Main wall — tight glow so walls don't appear fuzzy
        vr->drawGlowLineThin(w.a, w.b, col);

        // Bright core highlight
        Vec2 inA = w.a + dir * 2.f;
        Vec2 inB = w.b - dir * 2.f;
        if ((inB - inA).length() > 4.f)
            vr->drawGlowLineThin(inA, inB, core);

        // End-cap diamonds at each endpoint
        auto drawCap = [&](Vec2 c) {
            const float s = 5.f;
            vr->drawGlowLineThin(c + Vec2{0,-s}, c + Vec2{ s, 0}, col);
            vr->drawGlowLineThin(c + Vec2{ s, 0}, c + Vec2{0, s}, col);
            vr->drawGlowLineThin(c + Vec2{0, s}, c + Vec2{-s, 0}, col);
            vr->drawGlowLineThin(c + Vec2{-s, 0}, c + Vec2{0,-s}, col);
        };
        drawCap(w.a);
        drawCap(w.b);
    }
}

