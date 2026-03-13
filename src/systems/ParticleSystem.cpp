#include "ParticleSystem.hpp"
#include <algorithm>
#include <cmath>

ParticleSystem::ParticleSystem()
    : m_rng(std::random_device{}())
{
    m_pool.reserve(MAX);
    m_exhaustPool.reserve(MAX);
}

void ParticleSystem::emit(Vec2 pos, uint8_t r, uint8_t g, uint8_t b, int count) {
    std::uniform_real_distribution<float> angleDist(0.f, 6.28318f);
    std::uniform_real_distribution<float> speedDist(70.f, 260.f);
    std::uniform_real_distribution<float> lifeDist(0.4f, 0.9f);

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

void ParticleSystem::emitExhaust(Vec2 pos, Vec2 dir, Vec2 shipVel,
                                  uint8_t r, uint8_t g, uint8_t b, int count) {
    std::uniform_real_distribution<float> spreadDist(-0.35f, 0.35f); // ±20° cone
    std::uniform_real_distribution<float> speedDist(450.f, 750.f);
    std::uniform_real_distribution<float> lifeDist(0.15f, 0.40f);

    for (int i = 0; i < count && (int)m_exhaustPool.size() < MAX; ++i) {
        float a    = spreadDist(m_rng);
        float cosA = std::cos(a), sinA = std::sin(a);
        Vec2  v    = Vec2{ dir.x * cosA - dir.y * sinA,
                           dir.x * sinA + dir.y * cosA };
        float speed = speedDist(m_rng);
        float life  = lifeDist(m_rng);
        m_exhaustPool.push_back({ pos, v * speed, life, life, r, g, b });
    }
}

void ParticleSystem::emitThrust(Vec2 pos, Vec2 dir, uint8_t r, uint8_t g, uint8_t b, int count) {
    // dir must be normalised by caller; wider spread, higher speed, longer life
    std::uniform_real_distribution<float> spreadDist(-0.87f, 0.87f); // ±50°
    std::uniform_real_distribution<float> speedDist(130.f, 340.f);
    std::uniform_real_distribution<float> lifeDist(0.18f, 0.45f);

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
    constexpr float DRAG = 3.2f;
    for (auto& p : m_pool) {
        p.pos  += p.vel * dt;
        p.vel  *= std::max(0.f, 1.f - DRAG * dt);
        p.life -= dt;
    }
    m_pool.erase(
        std::remove_if(m_pool.begin(), m_pool.end(),
                       [](const Particle& p) { return p.life <= 0.f; }),
        m_pool.end());

    // Exhaust has higher drag — slows quickly so it clusters near nozzle
    constexpr float EXHAUST_DRAG = 4.5f;
    for (auto& p : m_exhaustPool) {
        p.pos  += p.vel * dt;
        p.vel  *= std::max(0.f, 1.f - EXHAUST_DRAG * dt);
        p.life -= dt;
    }
    m_exhaustPool.erase(
        std::remove_if(m_exhaustPool.begin(), m_exhaustPool.end(),
                       [](const Particle& p) { return p.life <= 0.f; }),
        m_exhaustPool.end());
}

void ParticleSystem::render(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);

    // Burst / explosion sparks — trail extends backward toward explosion origin
    for (const auto& p : m_pool) {
        float t     = p.life / p.maxLife;
        auto  alpha = static_cast<uint8_t>(t * 230);
        Vec2  tail  = p.pos - p.vel * 0.13f;

        SDL_SetRenderDrawColor(renderer, p.r, p.g, p.b, alpha);
        SDL_RenderDrawLineF(renderer, p.pos.x, p.pos.y, tail.x, tail.y);

        SDL_SetRenderDrawColor(renderer, p.r, p.g, p.b, alpha / 2);
        SDL_RenderDrawLineF(renderer, p.pos.x + 1, p.pos.y, tail.x + 1, tail.y);
        SDL_RenderDrawLineF(renderer, p.pos.x - 1, p.pos.y, tail.x - 1, tail.y);
    }

    // Exhaust — trail extends further backward away from ship (opposite direction of travel)
    for (const auto& p : m_exhaustPool) {
        float t     = p.life / p.maxLife;
        auto  alpha = static_cast<uint8_t>(t * t * 220);
        // tail is BEHIND the particle (further from ship): pos + vel*t extends away from nozzle
        Vec2  tail  = p.pos + p.vel * 0.04f;

        SDL_SetRenderDrawColor(renderer, p.r, p.g, p.b, alpha);
        SDL_RenderDrawLineF(renderer, p.pos.x, p.pos.y, tail.x, tail.y);

        SDL_SetRenderDrawColor(renderer, p.r, p.g, p.b, alpha / 2);
        SDL_RenderDrawLineF(renderer, p.pos.x + 1, p.pos.y, tail.x + 1, tail.y);
        SDL_RenderDrawLineF(renderer, p.pos.x - 1, p.pos.y, tail.x - 1, tail.y);
    }
}

void ParticleSystem::clear() {
    m_pool.clear();
    m_exhaustPool.clear();
}
