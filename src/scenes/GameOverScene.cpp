#include "GameOverScene.hpp"
#include "SceneContext.hpp"
#include "systems/ModSystem.hpp"
#include "systems/ProgramSystem.hpp"
#include "SceneManager.hpp"
#include "MapScene.hpp"
#include "world/NodeMap.hpp"
#include "core/Constants.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "math/Vec2.hpp"

#include <SDL.h>

void GameOverScene::onEnter(SceneContext&) {
    m_time = 0.f;
}

void GameOverScene::onExit() {}

static void returnToMap(SceneContext& ctx) {
    // Full run reset — wipe all persistent state
    if (ctx.mods)     ctx.mods->reset();
    if (ctx.programs) ctx.programs->reset();
    if (ctx.nodeMap)  ctx.nodeMap->reset();

    if (ctx.nodeMap)
        ctx.scenes->replace(std::make_unique<MapScene>(*ctx.nodeMap));
    else
        *ctx.running = false;
}

void GameOverScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    if (m_time < 1.5f) return; // brief lockout so accidental button press doesn't skip it

    if (ev.type == SDL_KEYDOWN) {
        returnToMap(ctx);
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        if (ev.cbutton.button == SDL_CONTROLLER_BUTTON_A
         || ev.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
            returnToMap(ctx);
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
    }

    SDL_RenderPresent(ctx.renderer);
}
