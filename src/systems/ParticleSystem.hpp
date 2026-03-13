#pragma once
#include "math/Vec2.hpp"
#include <SDL.h>
#include <cstdint>
#include <vector>
#include <random>

class ParticleSystem {
public:
    static constexpr int MAX = 256;

    ParticleSystem();

    // Burst of particles at pos, colored (r,g,b), count = number of sparks
    void emit(Vec2 pos, uint8_t r, uint8_t g, uint8_t b, int count = 10);

    // Directed exhaust stream — dir is the dominant exit direction, ±45° spread
    void emitThrust(Vec2 pos, Vec2 dir, uint8_t r, uint8_t g, uint8_t b, int count = 2);

    // Tight engine exhaust — ±15° cone, velocity-compensated so particles always trail behind
    void emitExhaust(Vec2 pos, Vec2 dir, Vec2 shipVel,
                     uint8_t r, uint8_t g, uint8_t b, int count = 2);

    void update(float dt);

    // Draw directly with SDL (short trail lines, alpha-faded by remaining life)
    void render(SDL_Renderer* renderer) const;

    void clear();

private:
    struct Particle {
        Vec2    pos;
        Vec2    vel;
        float   life;     // remaining seconds
        float   maxLife;
        uint8_t r, g, b;
    };

    std::vector<Particle> m_pool;        // burst / explosion sparks
    std::vector<Particle> m_exhaustPool; // engine exhaust (separate trail direction)
    std::mt19937          m_rng;
};
