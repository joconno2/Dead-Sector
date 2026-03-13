#include "VictoryScene.hpp"
#include "RunSummaryScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/Constants.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "math/Vec2.hpp"

#include <SDL.h>
#include <cmath>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Constructor / lifecycle
// ---------------------------------------------------------------------------

VictoryScene::VictoryScene(int score, int iceKilled, int nodesCleared, HullType hull)
    : m_score(score), m_iceKilled(iceKilled), m_nodesCleared(nodesCleared), m_hull(hull)
{}

void VictoryScene::onEnter(SceneContext& ctx) {
    m_time    = 0.f;
    m_canExit = false;

    HullStats stats = statsForHull(m_hull);
    m_bonus = 5000 + m_iceKilled * 50;

    if (ctx.saveData) {
        ctx.saveData->markGolden(stats.id);
        ctx.runCredits += m_bonus;
        ctx.runBonuses.push_back({ "BREACH BONUS", m_bonus });
        SaveSystem::save(*ctx.saveData);
    }
}

void VictoryScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    if (!m_canExit) return;

    bool pressed = false;
    if (ev.type == SDL_KEYDOWN)
        pressed = true;
    else if (ev.type == SDL_CONTROLLERBUTTONDOWN)
        pressed = (ev.cbutton.button == SDL_CONTROLLER_BUTTON_A ||
                   ev.cbutton.button == SDL_CONTROLLER_BUTTON_START);

    if (pressed)
        ctx.scenes->replace(std::make_unique<RunSummaryScene>(m_score, m_iceKilled, m_nodesCleared, true));
}

void VictoryScene::update(float dt, SceneContext&) {
    m_time += dt;
    if (m_time >= 2.5f) m_canExit = true;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void VictoryScene::renderGoldenShip(SDL_Renderer* r, float cx, float cy, float scale) const {
    auto verts = hullVerts(m_hull);
    if (verts.empty()) return;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    float pulse = 0.6f + 0.4f * std::sin(m_time * 2.5f);

    // Multi-layer gold glow (outermost to innermost)
    for (int layer = 8; layer >= 1; --layer) {
        float inf   = (float)layer * 1.8f;
        Uint8 alpha = (Uint8)(50.f * pulse / (layer * 0.5f + 0.5f));
        Uint8 g     = (Uint8)(140 + 60 * (1.f - layer / 8.f));
        SDL_SetRenderDrawColor(r, 255, g, 20, alpha);

        std::vector<SDL_Point> pts;
        for (auto& v : verts) {
            float dx = v.x * scale;
            float dy = v.y * scale;
            float len = std::sqrt(dx*dx + dy*dy);
            if (len > 0.f) { float f = (len + inf) / len; dx *= f; dy *= f; }
            pts.push_back({ (int)(cx + dx), (int)(cy + dy) });
        }
        pts.push_back(pts.front());
        SDL_RenderDrawLines(r, pts.data(), (int)pts.size());
    }

    // Core ship — bright gold
    std::vector<SDL_Point> core;
    for (auto& v : verts)
        core.push_back({ (int)(cx + v.x * scale), (int)(cy + v.y * scale) });
    core.push_back(core.front());

    SDL_SetRenderDrawColor(r, 255, (Uint8)(200 + 40 * pulse), 60, 255);
    SDL_RenderDrawLines(r, core.data(), (int)core.size());
    // Second pass brighter
    SDL_SetRenderDrawColor(r, 255, 240, 160, (Uint8)(180 * pulse));
    SDL_RenderDrawLines(r, core.data(), (int)core.size());

    // Exhaust particle trail (simple lines downward from ship)
    for (int i = 0; i < 8; ++i) {
        float t     = (float)i / 8.f;
        float phase = m_time * 4.f + t * 3.f;
        float x     = cx + (std::sin(phase) * 6.f * (1.f - t));
        float y1    = cy + 30.f * scale * 0.05f + t * 40.f;
        float y2    = y1 + 8.f;
        Uint8 a     = (Uint8)(200 * (1.f - t) * pulse);
        Uint8 gb    = (Uint8)(100 + 120 * (1.f - t));
        SDL_SetRenderDrawColor(r, 255, gb, 20, a);
        SDL_RenderDrawLine(r, (int)x, (int)y1, (int)x, (int)y2);
    }
}

void VictoryScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;
    if (!vr || !r) return;

    vr->clear();
    // Bright gold-tinted grid
    vr->drawGrid(60, 50, 20);
    vr->drawCRTOverlay();

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Animated reveal line
    float prog = std::min(m_time / 0.6f, 1.f);
    int lineX = (int)(prog * Constants::SCREEN_WF);
    if (lineX > 1 && vr) {
        GlowColor gc = { 255, 200, 30 };
        vr->drawGlowLine({0.f, Constants::SCREEN_HF * 0.5f},
                         {(float)lineX, Constants::SCREEN_HF * 0.5f}, gc);
    }

    if (!ctx.hud) { SDL_RenderPresent(r); return; }

    // Golden ship centred upper area
    float shipCX = Constants::SCREEN_WF * 0.5f;
    float shipCY = Constants::SCREEN_HF * 0.30f;
    renderGoldenShip(r, shipCX, shipCY, 2.8f);

    // Main panel
    int panelW = 520, panelH = 300;
    int panelX = Constants::SCREEN_W / 2 - panelW / 2;
    int panelY = (int)(Constants::SCREEN_HF * 0.52f);

    SDL_SetRenderDrawColor(r, 10, 18, 4, 220);
    SDL_Rect bg = { panelX, panelY, panelW, panelH };
    SDL_RenderFillRect(r, &bg);

    float bpulse = 0.5f + 0.5f * std::sin(m_time * 2.f);
    SDL_SetRenderDrawColor(r, 220, (Uint8)(160 + 60 * bpulse), 20, 220);
    SDL_RenderDrawRect(r, &bg);

    // Title
    HullStats stats = statsForHull(m_hull);
    SDL_Color titleCol = { 255, (Uint8)(180 + 60 * bpulse), 30, 255 };
    ctx.hud->drawLabel("BREACH SUCCESSFUL", panelX + 16, panelY + 14, titleCol);

    SDL_Color subCol = { 180, 120, 20, 180 };
    std::string hullStr = std::string("HULL: ") + stats.id + "  //  " + stats.name + "  [NOW GOLDEN]";
    ctx.hud->drawLabel(hullStr.c_str(), panelX + 16, panelY + 38, subCol);

    SDL_SetRenderDrawColor(r, 100, 70, 10, 140);
    SDL_RenderDrawLine(r, panelX+12, panelY+58, panelX+panelW-12, panelY+58);

    // Stats
    SDL_Color statCol = { 140, 200, 120, 255 };
    SDL_Color valCol  = { 220, 220, 80,  255 };
    SDL_Color bonCol  = { 255, 200, 50,  255 };

    int ry = panelY + 68;
    ctx.hud->drawLabel("ICE DESTROYED:", panelX+20, ry, statCol);
    ctx.hud->drawLabel(std::to_string(m_iceKilled).c_str(), panelX+280, ry, valCol); ry += 26;
    ctx.hud->drawLabel("NODES ACCESSED:", panelX+20, ry, statCol);
    ctx.hud->drawLabel(std::to_string(m_nodesCleared).c_str(), panelX+280, ry, valCol); ry += 26;
    ctx.hud->drawLabel("SCORE:", panelX+20, ry, statCol);
    ctx.hud->drawLabel(std::to_string(m_score).c_str(), panelX+280, ry, valCol); ry += 36;

    SDL_SetRenderDrawColor(r, 80, 60, 10, 120);
    SDL_RenderDrawLine(r, panelX+12, ry-6, panelX+panelW-12, ry-6);

    ctx.hud->drawLabel("BREACH BONUS:", panelX+20, ry, statCol);
    std::string bonusStr = "+" + std::to_string(m_bonus) + " CR";
    ctx.hud->drawLabel(bonusStr.c_str(), panelX+280, ry, bonCol); ry += 30;

    SDL_SetRenderDrawColor(r, 80, 60, 10, 120);
    SDL_RenderDrawLine(r, panelX+12, ry-4, panelX+panelW-12, ry-4);

    ctx.hud->drawLabel("HULL GILDED:", panelX+20, ry, statCol);
    std::string gildStr = std::string(stats.id) + " marked as GOLDEN";
    ctx.hud->drawLabel(gildStr.c_str(), panelX+190, ry, bonCol); ry += 30;

    // Prompt
    if (m_canExit) {
        float blink = 0.5f + 0.5f * std::sin(m_time * 3.f);
        SDL_Color promptCol = { 255, (Uint8)(160 + 60 * blink), 30, 220 };
        ctx.hud->drawLabel(">> PRESS ANY KEY FOR RUN SUMMARY <<", panelX+20, panelY+panelH-24, promptCol);
    }

    SDL_RenderPresent(r);
}
