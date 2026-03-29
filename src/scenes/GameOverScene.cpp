#include "GameOverScene.hpp"
#include "SceneContext.hpp"
#include "systems/ModSystem.hpp"
#include "systems/ProgramSystem.hpp"
#include "SceneManager.hpp"
#include "RunSummaryScene.hpp"
#include "world/NodeMap.hpp"
#include "core/Constants.hpp"
#include "core/SaveSystem.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "math/Vec2.hpp"

#include <SDL.h>

void GameOverScene::onEnter(SceneContext&) {
    m_time = 0.f;
}

void GameOverScene::onExit() {}

static void goToSummary(SceneContext& ctx, int score) {
    // Add score to this run's credits
    ctx.runCredits += score;

    int iceKilled    = ctx.runKills;
    int nodesCleared = ctx.runNodes;

    ctx.scenes->replace(std::make_unique<RunSummaryScene>(
        score, iceKilled, nodesCleared, false /*defeat*/));
}

void GameOverScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    if (m_time < 1.5f) return; // brief lockout

    if (ev.type == SDL_KEYDOWN || ev.type == SDL_MOUSEBUTTONDOWN) {
        goToSummary(ctx, m_score);
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        if (ev.cbutton.button == SDL_CONTROLLER_BUTTON_A
         || ev.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
            goToSummary(ctx, m_score);
        }
    }
}

void GameOverScene::update(float dt, SceneContext&) {
    m_time += dt;
}

void GameOverScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;

    vr->clear();
    vr->drawGrid();

    // Pulsing red FLATLINE across the screen
    float pulse = 0.5f + 0.5f * std::sin(m_time * 4.f);
    uint8_t rb = static_cast<uint8_t>(180 + 75 * pulse);
    Vec2 left  = {0.f,                   Constants::SCREEN_HF * 0.5f};
    Vec2 right = {Constants::SCREEN_WF,  Constants::SCREEN_HF * 0.5f};
    vr->drawGlowLine(left, right, {rb, 20, 20});

    vr->drawCRTOverlay();

    if (ctx.hud) {
        ctx.hud->render(m_score, 0.f);

        if (m_time > 1.5f) {
            SDL_Color promptCol = { 220, 80, 60, 200 };
            const std::string prompt = ">> PRESS ANY KEY <<";
            ctx.hud->drawLabel(prompt,
                               Constants::SCREEN_W / 2 - ctx.hud->measureText(prompt) / 2,
                               Constants::SCREEN_H / 2 + 40,
                               promptCol);
        }
    }

    SDL_RenderPresent(ctx.renderer);
}
