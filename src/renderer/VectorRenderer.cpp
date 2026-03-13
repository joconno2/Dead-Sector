#include "VectorRenderer.hpp"
#include "core/Constants.hpp"
#include <SDL.h>

VectorRenderer::VectorRenderer(SDL_Renderer* renderer)
    : m_renderer(renderer)
{
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
}

void VectorRenderer::clear() {
    SDL_SetRenderDrawColor(m_renderer, 2, 4, 12, 255);
    SDL_RenderClear(m_renderer);
}

void VectorRenderer::drawLine(Vec2 a, Vec2 b, uint8_t r, uint8_t g, uint8_t b_ch, uint8_t alpha) {
    SDL_SetRenderDrawColor(m_renderer, r, g, b_ch, alpha);
    SDL_RenderDrawLineF(m_renderer, a.x, a.y, b.x, b.y);
}

void VectorRenderer::drawGlowLine(Vec2 a, Vec2 b, GlowColor col) {
    Vec2 delta = (b - a);
    float len = delta.length();
    if (len < 0.001f) return;

    // Perpendicular unit vector for parallel offset passes
    Vec2 perp = Vec2{-delta.y / len, delta.x / len};

    // Sub-pixel stepped Gaussian halo — 0.5px increments reduce aliasing staircase
    // because each step rounds to a different pixel on diagonal lines, distributing
    // the energy smoothly rather than landing on the same pixel column repeatedly.
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_ADD);

    // Gaussian bloom — integer steps starting at 1px so each pass hits distinct pixels.
    // 0.5px sub-pixel steps are wasted (SDL rounds to the same pixel as the core).
    // Wide extent (7px) with strong per-step alpha makes the falloff clearly visible.
    struct { float r; uint8_t a; } levels[] = {
        { 1.f, 140 },
        { 2.f,  90 },
        { 3.f,  55 },
        { 4.f,  32 },
        { 5.f,  18 },
        { 6.f,  10 },
        { 7.f,   5 },
    };
    for (auto& lv : levels) {
        drawLine(a + perp * lv.r, b + perp * lv.r, col.r, col.g, col.b, lv.a);
        drawLine(a - perp * lv.r, b - perp * lv.r, col.r, col.g, col.b, lv.a);
    }

    // White-hot core at full brightness
    drawLine(a, b, col.r, col.g, col.b, 255);
}

void VectorRenderer::drawGlowLineThin(Vec2 a, Vec2 b, GlowColor col) {
    Vec2 delta = (b - a);
    float len = delta.length();
    if (len < 0.001f) return;
    Vec2 perp = Vec2{-delta.y / len, delta.x / len};

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_ADD);
    drawLine(a + perp * 1.f, b + perp * 1.f, col.r, col.g, col.b, 80);
    drawLine(a - perp * 1.f, b - perp * 1.f, col.r, col.g, col.b, 80);
    drawLine(a + perp * 2.f, b + perp * 2.f, col.r, col.g, col.b, 35);
    drawLine(a - perp * 2.f, b - perp * 2.f, col.r, col.g, col.b, 35);
    drawLine(a + perp * 3.f, b + perp * 3.f, col.r, col.g, col.b, 12);
    drawLine(a - perp * 3.f, b - perp * 3.f, col.r, col.g, col.b, 12);
    drawLine(a, b, col.r, col.g, col.b, 255);
}

void VectorRenderer::drawGlowPoly(const std::vector<Vec2>& verts, GlowColor color) {
    if (verts.size() < 2) return;
    for (size_t i = 0; i < verts.size(); ++i) {
        drawGlowLine(verts[i], verts[(i + 1) % verts.size()], color);
    }
}

void VectorRenderer::drawGrid(uint8_t r, uint8_t g, uint8_t b) {
    constexpr int SPACING = 80;
    SDL_SetRenderDrawColor(m_renderer, r, g, b, 52);
    for (int x = 0; x <= Constants::SCREEN_W; x += SPACING)
        SDL_RenderDrawLine(m_renderer, x, 0, x, Constants::SCREEN_H);
    for (int y = 0; y <= Constants::SCREEN_H; y += SPACING)
        SDL_RenderDrawLine(m_renderer, 0, y, Constants::SCREEN_W, y);
}

void VectorRenderer::drawGlitch(float intensity, Uint32 tick) {
    if (intensity < 0.01f) return;
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

    int numBands = 1 + (int)(intensity * 4.f);
    for (int i = 0; i < numBands; ++i) {
        // Band drifts slowly using tick as an evolving seed
        int y = (int)((tick / 4 + (Uint32)(i * 1987)) % (Uint32)Constants::SCREEN_H);
        int h = 1 + (int)(intensity * 7.f);

        // Mostly cyan/green, occasional red flash at high intensity
        bool redFlash = (intensity > 0.6f) && (((tick / 8) + i) % 45 == 0);
        uint8_t rc = redFlash ? 255 :  0;
        uint8_t gc = redFlash ?  20 : 200;
        uint8_t bc = redFlash ?  20 : 120;
        uint8_t ac = (uint8_t)(20 + intensity * 70);

        SDL_SetRenderDrawColor(m_renderer, rc, gc, bc, ac);
        SDL_Rect band = { 0, y, Constants::SCREEN_W, h };
        SDL_RenderFillRect(m_renderer, &band);

        // Bright leading edge line
        SDL_SetRenderDrawColor(m_renderer, 180, 255, 200,
            (uint8_t)(intensity * 160));
        SDL_RenderDrawLine(m_renderer, 0, y, Constants::SCREEN_W, y);
    }

    // At very high intensity: occasional full-width bright flash line
    if (intensity > 0.8f && (tick % 90 < 3)) {
        int fy = (int)((tick * 7) % Constants::SCREEN_H);
        SDL_SetRenderDrawColor(m_renderer, 100, 255, 180, 80);
        SDL_RenderDrawLine(m_renderer, 0, fy, Constants::SCREEN_W, fy);
        SDL_RenderDrawLine(m_renderer, 0, fy+1, Constants::SCREEN_W, fy+1);
    }
}

void VectorRenderer::drawCRTOverlay() {
    // Subtle scanlines: every 4 pixels, low alpha — just a hint of CRT texture
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 14);
    for (int y = 0; y < Constants::SCREEN_H; y += 4) {
        SDL_RenderDrawLine(m_renderer, 0, y, Constants::SCREEN_W, y);
    }
}
