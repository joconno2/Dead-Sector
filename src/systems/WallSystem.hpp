#pragma once
#include "math/Vec2.hpp"
#include <vector>
#include <cstdint>

struct Wall {
    Vec2    a, b;       // endpoints
    Vec2    normal;     // unit normal perpendicular to segment (either direction)
};

class WallSystem {
public:
    // Generate walls for a node. nodeId used as RNG seed for repeatability.
    void generate(int tier, int nodeId);
    void clear() { m_walls.clear(); }

    const std::vector<Wall>& walls() const { return m_walls; }
    bool empty() const { return m_walls.empty(); }

    // Resolve a circle (pos, radius) against all walls.
    // Reflects velocity and pushes pos out of penetration.
    // Returns true if any collision occurred.
    bool resolveCircle(Vec2& pos, Vec2& vel, float radius, float energyLoss = 0.85f) const;

    // Render walls as glowing line segments
    void render(struct VectorRenderer* vr, uint8_t r, uint8_t g, uint8_t b) const;

private:
    std::vector<Wall> m_walls;

    static Vec2 closestPointOnSegment(Vec2 p, Vec2 a, Vec2 b);
};
