#include "ParticleSystem.hpp"
#include <algorithm>
#include <cmath>

ParticleSystem::ParticleSystem()
    : m_rng(std::random_device{}())
{
    m_pool.reserve(MAX);
}

void ParticleSystem::emit(Vec2 pos, uint8_t r, uint8_t g, uint8_t b, int count) {
    std::uniform_real_distribution<float> angleDist(0.f, 6.28318f);
    std::uniform_real_distribution<float> speedDist(50.f, 200.f);
    std::uniform_real_distribution<float> lifeDist(0.35f, 0.75f);

    for (int i = 0; i < count && (int)m_pool.size() < MAX; ++i) {
        float angle = angleDist(m_rng);
        float speed = speedDist(m_rng);
        float life  = lifeDist(m_rng);
        m_pool.push_back({
            pos,
            Vec2{ std::cos(angle) * speed, std::sin(angle) * speed },
            life, life, r, g, b
        });
    }
}

void ParticleSystem::emitThrust(Vec2 pos, Vec2 dir, uint8_t r, uint8_t g, uint8_t b, int count) {
    // dir must be normalised by caller; wider spread, higher speed, longer life
    std::uniform_real_distribution<float> spreadDist(-1.48f, 1.48f); // ±85°
    std::uniform_real_distribution<float> speedDist(150.f, 420.f);
    std::uniform_real_distribution<float> lifeDist(0.18f, 0.52f);

    for (int i = 0; i < count && (int)m_pool.size() < MAX; ++i) {
        float a    = spreadDist(m_rng);
        float cosA = std::cos(a), sinA = std::sin(a);
        Vec2  v    = Vec2{ dir.x * cosA - dir.y * sinA,
                           dir.x * sinA + dir.y * cosA };
        float speed = speedDist(m_rng);
        float life  = lifeDist(m_rng);
        m_pool.push_back({ pos, v * speed, life, life, r, g, b });
    }
}

void ParticleSystem::update(float dt) {
    constexpr float DRAG = 3.5f;
    for (auto& p : m_pool) {
        p.pos  += p.vel * dt;
        p.vel  *= std::max(0.f, 1.f - DRAG * dt);
        p.life -= dt;
    }
    m_pool.erase(
        std::remove_if(m_pool.begin(), m_pool.end(),
                       [](const Particle& p) { return p.life <= 0.f; }),
        m_pool.end());
}

void ParticleSystem::render(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (const auto& p : m_pool) {
        float t    = p.life / p.maxLife;           // 1=fresh, 0=dead
        auto  alpha = static_cast<uint8_t>(t * 200);

        // Trail: draw from current pos back along velocity
        Vec2 tail = p.pos - p.vel * 0.09f;

        // Core bright line
        SDL_SetRenderDrawColor(renderer, p.r, p.g, p.b, alpha);
        SDL_RenderDrawLineF(renderer, p.pos.x, p.pos.y, tail.x, tail.y);

        // Glow halo (wider, dimmer)
        SDL_SetRenderDrawColor(renderer, p.r, p.g, p.b, alpha / 3);
        SDL_RenderDrawLineF(renderer, p.pos.x + 1, p.pos.y,
                                      tail.x  + 1, tail.y);
        SDL_RenderDrawLineF(renderer, p.pos.x - 1, p.pos.y,
                                      tail.x  - 1, tail.y);
    }
}

void ParticleSystem::clear() {
    m_pool.clear();
}
