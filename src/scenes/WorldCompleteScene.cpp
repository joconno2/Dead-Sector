#include "WorldCompleteScene.hpp"
#include "MapScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/Constants.hpp"
#include "core/Worlds.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "world/NodeMap.hpp"

#include <SDL.h>
#include <cmath>
#include <string>

WorldCompleteScene::WorldCompleteScene(int worldId, int score, int iceKilled)
    : m_worldId(worldId), m_score(score), m_iceKilled(iceKilled)
{}

void WorldCompleteScene::onEnter(SceneContext& ctx) {
    m_time    = 0.f;
    m_canExit = false;

    // Unlock the next world persistently
    if (ctx.saveData) {
        int needed = m_worldId + 2;  // beating world 0 → unlock world 1 (worldsUnlocked=2)
        if (ctx.saveData->worldsUnlocked < needed)
            ctx.saveData->worldsUnlocked = needed;
        SaveSystem::save(*ctx.saveData);
    }
}

void WorldCompleteScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    if (!m_canExit) return;

    bool pressed = false;
    if (ev.type == SDL_KEYDOWN)
        pressed = true;
    else if (ev.type == SDL_CONTROLLERBUTTONDOWN)
        pressed = (ev.cbutton.button == SDL_CONTROLLER_BUTTON_A ||
                   ev.cbutton.button == SDL_CONTROLLER_BUTTON_START);

    if (pressed) advance(ctx);
}

void WorldCompleteScene::update(float dt, SceneContext&) {
    m_time += dt;
    if (m_time >= 2.5f) m_canExit = true;
}

void WorldCompleteScene::advance(SceneContext& ctx) {
    ctx.currentWorld = m_worldId + 1;
    if (ctx.nodeMap) {
        ctx.nodeMap->reset(ctx.currentWorld);
        ctx.scenes->replace(std::make_unique<MapScene>(*ctx.nodeMap));
    }
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

// Draw a glowing ring (approximated as polygon)
static void drawGlowRing(SDL_Renderer* r, float cx, float cy, float radius,
                         int sides, uint8_t rr, uint8_t gg, uint8_t bb, uint8_t alpha) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, rr, gg, bb, alpha);
    for (int i = 0; i < sides; ++i) {
        float a0 = (float)i       / sides * 6.28318f;
        float a1 = (float)(i + 1) / sides * 6.28318f;
        int x0 = (int)(cx + std::cos(a0) * radius);
        int y0 = (int)(cy + std::sin(a0) * radius);
        int x1 = (int)(cx + std::cos(a1) * radius);
        int y1 = (int)(cy + std::sin(a1) * radius);
        SDL_RenderDrawLine(r, x0, y0, x1, y1);
    }
}

void WorldCompleteScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;
    if (!vr || !r) return;

    const WorldDef& wd  = worldDef(m_worldId);
    const WorldDef& wd2 = worldDef(m_worldId + 1);

    vr->clear();
    vr->drawGrid(wd.theme.gridR, wd.theme.gridG, wd.theme.gridB);
    vr->drawCRTOverlay();

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Animated reveal line
    float prog = std::min(m_time / 0.5f, 1.f);
    {
        GlowColor gc = { wd.theme.accentR, wd.theme.accentG, wd.theme.accentB };
        vr->drawGlowLine({0.f, Constants::SCREEN_HF * 0.5f},
                         {prog * Constants::SCREEN_WF, Constants::SCREEN_HF * 0.5f}, gc);
    }

    // Decorative expanding rings from centre
    float pulse = 0.5f + 0.5f * std::sin(m_time * 3.f);
    float cx = Constants::SCREEN_WF * 0.5f;
    float cy = Constants::SCREEN_HF * 0.38f;
    for (int ring = 1; ring <= 3; ++ring) {
        float rr2 = 30.f + ring * 28.f + std::sin(m_time * 2.f + ring) * 6.f;
        uint8_t aa = (uint8_t)(60 * pulse / ring);
        drawGlowRing(r, cx, cy, rr2, 32,
                     wd.theme.accentR, wd.theme.accentG, wd.theme.accentB, aa);
    }

    if (!ctx.hud) { SDL_RenderPresent(r); return; }

    // Panel
    constexpr int PW = 580, PH = 280;
    int px = Constants::SCREEN_W / 2 - PW / 2;
    int py = (int)(Constants::SCREEN_HF * 0.52f);

    SDL_SetRenderDrawColor(r, 4, 8, 18, 230);
    SDL_Rect bg = { px, py, PW, PH };
    SDL_RenderFillRect(r, &bg);

    Uint8 br = (Uint8)(wd.theme.accentR / 2 + wd.theme.accentR / 2 * pulse);
    Uint8 bg2 = (Uint8)(wd.theme.accentG / 2 + wd.theme.accentG / 2 * pulse);
    Uint8 bb  = (Uint8)(wd.theme.accentB / 2 + wd.theme.accentB / 2 * pulse);
    SDL_SetRenderDrawColor(r, br, bg2, bb, 220);
    SDL_RenderDrawRect(r, &bg);

    // Title
    SDL_Color titleCol = { wd.theme.accentR, (uint8_t)(wd.theme.accentG + (uint8_t)(20 * pulse)),
                           wd.theme.accentB, 255 };
    std::string title = std::string(wd.name) + " BREACHED";
    ctx.hud->drawLabel(title.c_str(), px + 16, py + 14, titleCol);

    SDL_Color dimCol = { 120, 160, 130, 180 };
    SDL_SetRenderDrawColor(r, 60, 80, 70, 120);
    SDL_RenderDrawLine(r, px + 12, py + 46, px + PW - 12, py + 46);

    // Stats
    SDL_Color statCol = { 120, 190, 150, 255 };
    SDL_Color valCol  = { 220, 230, 100, 255 };
    int ry = py + 58;
    ctx.hud->drawLabel("ICE DESTROYED:", px + 20, ry, statCol);
    ctx.hud->drawLabel(std::to_string(m_iceKilled).c_str(), px + 280, ry, valCol); ry += 26;
    ctx.hud->drawLabel("SCORE:", px + 20, ry, statCol);
    ctx.hud->drawLabel(std::to_string(m_score).c_str(), px + 280, ry, valCol); ry += 40;

    SDL_SetRenderDrawColor(r, 60, 80, 70, 120);
    SDL_RenderDrawLine(r, px + 12, ry - 8, px + PW - 12, ry - 8);

    // Next world announcement
    SDL_Color nextCol = { wd2.theme.accentR, wd2.theme.accentG, wd2.theme.accentB, 255 };
    std::string connecting = std::string("CONNECTING TO: ") + wd2.name;
    ctx.hud->drawLabel(connecting.c_str(), px + 20, ry, nextCol); ry += 26;

    ctx.hud->drawLabel(wd2.subtitle, px + 20, ry, dimCol); ry += 26;
    ctx.hud->drawLabel(wd2.flavor,   px + 20, ry, { 80, 110, 90, 160 });

    // Prompt
    if (m_canExit) {
        float blink = 0.5f + 0.5f * std::sin(m_time * 3.5f);
        SDL_Color promptCol = { wd2.theme.accentR,
                                (uint8_t)(wd2.theme.accentG * blink),
                                wd2.theme.accentB, 220 };
        ctx.hud->drawLabel(">> PRESS ANY KEY TO JACK IN <<", px + 20, py + PH - 26, promptCol);
    }

    SDL_RenderPresent(r);
}
