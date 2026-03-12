#pragma once
#include "math/Vec2.hpp"
#include <SDL.h>
#include <cstdint>
#include <vector>

struct GlowColor {
    uint8_t r, g, b;
};

class VectorRenderer {
public:
    explicit VectorRenderer(SDL_Renderer* renderer);

    // Clear to near-black background
    void clear();

    // Draw a closed polygon with glow (last vert connects back to first)
    void drawGlowPoly(const std::vector<Vec2>& verts, GlowColor color);

    // Draw a single line segment with glow
    void drawGlowLine(Vec2 a, Vec2 b, GlowColor color);

    // Draw CRT scanline overlay (call last before SDL_RenderPresent)
    void drawCRTOverlay();

    // Draw background grid — optional RGB tint for per-node color theming
    void drawGrid(uint8_t r = 20, uint8_t g = 50, uint8_t b = 80);

    // Draw glitch artifact strips — intensity 0..1, call after CRTOverlay
    void drawGlitch(float intensity, Uint32 tick);

private:
    SDL_Renderer* m_renderer;

    void drawLine(Vec2 a, Vec2 b, uint8_t r, uint8_t g, uint8_t b_ch, uint8_t alpha);
};
