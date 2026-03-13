#include "FragmentSystem.hpp"
#include <random>
#include <cmath>
#include <algorithm>

void FragmentSystem::emit(Vec2 pos, uint8_t r, uint8_t g, uint8_t b,
                          int count, float baseLen, float spreadSpeed)
{
    if ((int)m_frags.size() >= MAX) return;

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> angleDist(0.f, 6.28318f);
    std::uniform_real_distribution<float> speedMult(0.45f, 1.15f);
    std::uniform_real_distribution<float> lenMult(0.6f, 1.6f);
    std::uniform_real_distribution<float> angVelDist(-10.f, 10.f);
    std::uniform_real_distribution<float> lifeDist(0.5f, 1.0f);

    int room = MAX - (int)m_frags.size();
    count = std::min(count, room);

    for (int i = 0; i < count; ++i) {
        float a   = angleDist(rng);
        float spd = spreadSpeed * speedMult(rng);
        float life = lifeDist(rng);
        m_frags.push_back({
            pos,
            Vec2{ std::cos(a) * spd, std::sin(a) * spd },
            a,
            angVelDist(rng),
            baseLen * lenMult(rng),
            life, life,
            r, g, b
        });
    }
}

void FragmentSystem::update(float dt) {
    constexpr float DRAG = 2.5f;
    for (auto& f : m_frags) {
        f.pos   += f.vel * dt;
        f.vel   *= std::max(0.f, 1.f - DRAG * dt);
        f.angle += f.angVel * dt;
        f.life  -= dt;
    }
    m_frags.erase(
        std::remove_if(m_frags.begin(), m_frags.end(),
                       [](const Fragment& f){ return f.life <= 0.f; }),
        m_frags.end());
}

void FragmentSystem::render(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    for (const auto& f : m_frags) {
        float t = f.life / f.maxLife;   // 1=fresh → 0=dead
        auto alpha = static_cast<uint8_t>(t * t * 255);

        float cx = std::cos(f.angle), sx = std::sin(f.angle);
        float ax = f.pos.x + cx * f.len,  ay = f.pos.y + sx * f.len;
        float bx = f.pos.x - cx * f.len,  by = f.pos.y - sx * f.len;

        // Bright core line
        SDL_SetRenderDrawColor(renderer, f.r, f.g, f.b, alpha);
        SDL_RenderDrawLineF(renderer, ax, ay, bx, by);

        // Glow halo — half-dim on all four sides for more body
        uint8_t glow = alpha / 2;
        SDL_SetRenderDrawColor(renderer, f.r, f.g, f.b, glow);
        SDL_RenderDrawLineF(renderer, ax+1, ay, bx+1, by);
        SDL_RenderDrawLineF(renderer, ax-1, ay, bx-1, by);
        SDL_RenderDrawLineF(renderer, ax, ay+1, bx, by+1);
        SDL_RenderDrawLineF(renderer, ax, ay-1, bx, by-1);
    }
}

void FragmentSystem::clear() {
    m_frags.clear();
}
