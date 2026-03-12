#pragma once
#include "math/Vec2.hpp"
#include <SDL.h>
#include <vector>
#include <cstdint>

struct Fragment {
    Vec2    pos;
    Vec2    vel;
    float   angle;
    float   angVel;   // radians/sec rotation
    float   len;      // half-length of the line segment
    float   life;
    float   maxLife;
    uint8_t r, g, b;
};

class FragmentSystem {
public:
    // Emit fragments at pos matching a given enemy type geometry
    // count = number of fragments, baseLen = half-length of each segment
    void emit(Vec2 pos, uint8_t r, uint8_t g, uint8_t b, int count, float baseLen,
              float spreadSpeed = 280.f);

    void update(float dt);
    void render(SDL_Renderer* renderer) const;
    void clear();

private:
    static constexpr int MAX = 512;
    std::vector<Fragment> m_frags;
};
