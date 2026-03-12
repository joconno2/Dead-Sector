#include "RunSummaryScene.hpp"
#include "MainMenuScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/Constants.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "world/NodeMap.hpp"
#include "systems/ModSystem.hpp"
#include "systems/ProgramSystem.hpp"

#include <SDL.h>
#include <cmath>
#include <string>

RunSummaryScene::RunSummaryScene(int score, int iceKilled, int nodesCleared, bool victory)
    : m_score(score)
    , m_iceKilled(iceKilled)
    , m_nodesCleared(nodesCleared)
    , m_victory(victory)
{}

void RunSummaryScene::onEnter(SceneContext& ctx) {
    m_time    = 0.f;
    m_canExit = false;

    // Commit run stats to save data
    if (ctx.saveData) {
        ctx.saveData->credits    += ctx.runCredits;
        ctx.saveData->totalRuns  += 1;
        ctx.saveData->totalKills += ctx.runKills;
        if (m_score > ctx.saveData->highScore)
            ctx.saveData->highScore = m_score;
        SaveSystem::save(*ctx.saveData);
    }

    // Reset run state
    if (ctx.mods)     ctx.mods->reset();
    if (ctx.programs) ctx.programs->reset();
    if (ctx.nodeMap)  ctx.nodeMap->reset();
}

void RunSummaryScene::onExit() {}

static void goToMainMenu(SceneContext& ctx) {
    // Clear per-run state before returning to menu
    ctx.runCredits  = 0;
    ctx.runKills    = 0;
    ctx.runNodes    = 0;
    ctx.endlessMode = false;
    ctx.endlessWave = 0;
    ctx.runBonuses.clear();

    ctx.scenes->replace(std::make_unique<MainMenuScene>());
}

void RunSummaryScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    if (!m_canExit) return;

    bool pressed = false;
    if (ev.type == SDL_KEYDOWN)
        pressed = true;
    else if (ev.type == SDL_CONTROLLERBUTTONDOWN)
        pressed = (ev.cbutton.button == SDL_CONTROLLER_BUTTON_A
                || ev.cbutton.button == SDL_CONTROLLER_BUTTON_START);

    if (pressed) goToMainMenu(ctx);
}

void RunSummaryScene::update(float dt, SceneContext&) {
    m_time += dt;
    if (m_time >= 2.0f) m_canExit = true;
}

void RunSummaryScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;

    vr->clear();
    vr->drawGrid(m_victory ? 20 : 50, m_victory ? 60 : 10, m_victory ? 40 : 10);

    // Animated horizontal line
    float progress = std::min(m_time / 0.8f, 1.f);
    int   lineX    = (int)(progress * Constants::SCREEN_WF);
    if (lineX > 1) {
        Vec2 a = { 0.f, Constants::SCREEN_HF * 0.5f };
        Vec2 b = { (float)lineX, Constants::SCREEN_HF * 0.5f };
        GlowColor lc = m_victory ? GlowColor{0, 220, 100} : GlowColor{220, 30, 30};
        vr->drawGlowLine(a, b, lc);
    }
    vr->drawCRTOverlay();

    if (!ctx.hud) { SDL_RenderPresent(r); return; }

    // Main panel
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    int panelW = 520;
    int panelH = 340;
    int panelX = Constants::SCREEN_W / 2 - panelW / 2;
    int panelY = Constants::SCREEN_H / 2 - panelH / 2;

    SDL_SetRenderDrawColor(r, 0, 20, 12, 220);
    SDL_Rect bg = { panelX, panelY, panelW, panelH };
    SDL_RenderFillRect(r, &bg);

    SDL_Color borderCol = m_victory ? SDL_Color{0, 200, 100, 240} : SDL_Color{220, 30, 30, 240};
    SDL_SetRenderDrawColor(r, borderCol.r, borderCol.g, borderCol.b, borderCol.a);
    SDL_RenderDrawRect(r, &bg);

    // Title
    const char* title = m_victory ? "EXTRACTION COMPLETE" : "RUN TERMINATED";
    SDL_Color titleCol = m_victory ? SDL_Color{0, 220, 100, 255} : SDL_Color{220, 30, 30, 255};
    ctx.hud->drawLabel(title, panelX + 16, panelY + 14, titleCol);

    // Separator line
    SDL_SetRenderDrawColor(r, borderCol.r / 2, borderCol.g / 2, borderCol.b / 2, 160);
    SDL_RenderDrawLine(r, panelX + 12, panelY + 48, panelX + panelW - 12, panelY + 48);

    // Stats
    SDL_Color statCol    = { 140, 200, 160, 255 };
    SDL_Color valCol     = { 220, 220, 80,  255 };
    SDL_Color bonusLabel = { 100, 180, 220, 240 };
    SDL_Color bonusVal   = { 80,  220, 180, 240 };

    int rowY = panelY + 58;
    ctx.hud->drawLabel("ICE DESTROYED:",  panelX + 20, rowY,       statCol);
    ctx.hud->drawLabel(std::to_string(m_iceKilled),    panelX + 280, rowY, valCol);
    rowY += 26;
    ctx.hud->drawLabel("NODES ACCESSED:", panelX + 20, rowY,       statCol);
    ctx.hud->drawLabel(std::to_string(m_nodesCleared), panelX + 280, rowY, valCol);
    rowY += 26;
    ctx.hud->drawLabel("SCORE:",           panelX + 20, rowY,       statCol);
    ctx.hud->drawLabel(std::to_string(m_score),        panelX + 280, rowY, valCol);
    rowY += 36;

    // Separator
    SDL_SetRenderDrawColor(r, 30, 100, 60, 100);
    SDL_RenderDrawLine(r, panelX + 12, rowY - 6, panelX + panelW - 12, rowY - 6);

    // Credit bonuses
    ctx.hud->drawLabel("// CREDIT TRANSACTIONS //", panelX + 20, rowY, {80, 160, 130, 200});
    rowY += 22;

    int totalShown = 0;
    for (const auto& bonus : ctx.runBonuses) {
        if (totalShown >= 5) break; // cap display
        ctx.hud->drawLabel(bonus.name, panelX + 20, rowY, bonusLabel);
        ctx.hud->drawLabel("+" + std::to_string(bonus.amount) + " CR",
                           panelX + 340, rowY, bonusVal);
        rowY += 20;
        ++totalShown;
    }
    if ((int)ctx.runBonuses.size() > 5) {
        ctx.hud->drawLabel("  ...and more", panelX + 20, rowY, {80, 130, 100, 180});
        rowY += 20;
    }

    // Total credits
    rowY += 8;
    SDL_SetRenderDrawColor(r, 30, 140, 80, 100);
    SDL_RenderDrawLine(r, panelX + 12, rowY - 4, panelX + panelW - 12, rowY - 4);
    ctx.hud->drawLabel("TOTAL CREDITS BANKED:", panelX + 20, rowY, {0, 220, 160, 255});
    ctx.hud->drawLabel(std::to_string(ctx.runCredits) + " CR",
                       panelX + 310, rowY, {0, 255, 180, 255});
    rowY += 30;

    // Prompt
    if (m_canExit) {
        float blink = 0.5f + 0.5f * std::sin(m_time * 3.f);
        SDL_Color promptCol = { 0, (uint8_t)(160 + 60 * blink), (uint8_t)(140 + 40 * blink), 220 };
        ctx.hud->drawLabel(">> PRESS ANY KEY <<", panelX + 20, rowY, promptCol);
    }

    // High score / save data info
    if (ctx.saveData) {
        std::string hsStr = "HIGH SCORE: " + std::to_string(ctx.saveData->highScore);
        std::string totalCredStr = "TOTAL CREDITS: " + std::to_string(ctx.saveData->credits) + " CR";
        ctx.hud->drawLabel(hsStr,        panelX + 20, panelY + panelH - 40, {100, 140, 120, 180});
        ctx.hud->drawLabel(totalCredStr, panelX + 20, panelY + panelH - 20, {80,  180, 120, 200});
    }

    SDL_RenderPresent(r);
}
