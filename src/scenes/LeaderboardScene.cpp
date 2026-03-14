#include "LeaderboardScene.hpp"
#include "MainMenuScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/Constants.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"

#include <SDL.h>
#include <cmath>
#include <string>

void LeaderboardScene::onEnter(SceneContext&) {
    m_time    = 0.f;
    m_canExit = false;
}

void LeaderboardScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    if (!m_canExit) return;
    bool pressed = false;
    if (ev.type == SDL_KEYDOWN)
        pressed = true;
    else if (ev.type == SDL_CONTROLLERBUTTONDOWN)
        pressed = (ev.cbutton.button == SDL_CONTROLLER_BUTTON_B
                || ev.cbutton.button == SDL_CONTROLLER_BUTTON_START);
    if (pressed)
        ctx.scenes->replace(std::make_unique<MainMenuScene>());
}

void LeaderboardScene::update(float dt, SceneContext&) {
    m_time += dt;
    if (m_time > 0.4f) m_canExit = true;
}

void LeaderboardScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;

    vr->clear();
    vr->drawGrid(10, 15, 40);
    vr->drawCRTOverlay();

    if (!ctx.hud || !ctx.saveData) { SDL_RenderPresent(r); return; }

    const SaveData& sd = *ctx.saveData;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Title
    const char* title = "// HIGH SCORES //";
    int titleW = ctx.hud->measureText(title);
    ctx.hud->drawLabel(title, Constants::SCREEN_W / 2 - titleW / 2, 28, {0, 220, 170, 255});

    // Sub-label: player callsign
    std::string sub = "CALLSIGN: " + sd.playerName;
    int subW = ctx.hud->measureText(sub);
    ctx.hud->drawLabel(sub, Constants::SCREEN_W / 2 - subW / 2, 56, {80, 160, 130, 200});

    // Two panels side by side
    constexpr int PANEL_W  = 440;
    constexpr int PANEL_H  = 340;
    constexpr int PANEL_PAD = 30;
    int totalW = PANEL_W * 2 + PANEL_PAD;
    int px0    = Constants::SCREEN_W / 2 - totalW / 2;
    int py0    = 90;

    auto drawPanel = [&](int px, int py, bool endless) {
        const auto& list = endless ? sd.endlessScores : sd.normalScores;
        const char* heading = endless ? "ENDLESS" : "NORMAL RUNS";

        SDL_SetRenderDrawColor(r, 0, endless ? 10 : 20, endless ? 20 : 10, 200);
        SDL_Rect bg = { px, py, PANEL_W, PANEL_H };
        SDL_RenderFillRect(r, &bg);

        SDL_Color borCol = endless ? SDL_Color{80, 60, 255, 200} : SDL_Color{0, 200, 130, 200};
        SDL_SetRenderDrawColor(r, borCol.r, borCol.g, borCol.b, borCol.a);
        SDL_RenderDrawRect(r, &bg);

        // Heading
        ctx.hud->drawLabel(heading, px + 16, py + 12, borCol);
        SDL_SetRenderDrawColor(r, borCol.r/2, borCol.g/2, borCol.b/2, 140);
        SDL_RenderDrawLine(r, px+8, py+38, px+PANEL_W-8, py+38);

        // Column headers
        SDL_Color hdrCol = { 100, 140, 120, 200 };
        ctx.hud->drawLabel("#",      px + 16,  py + 46, hdrCol);
        ctx.hud->drawLabel("CALLSIGN", px + 42, py + 46, hdrCol);
        ctx.hud->drawLabel("SCORE",  px + 220, py + 46, hdrCol);
        if (endless)
            ctx.hud->drawLabel("WAVE", px + 360, py + 46, hdrCol);

        SDL_RenderDrawLine(r, px+8, py+66, px+PANEL_W-8, py+66);

        if (list.empty()) {
            SDL_Color empty = { 60, 80, 70, 160 };
            ctx.hud->drawLabel("-- NO RUNS RECORDED --", px + 16, py + 80, empty);
        }

        for (int i = 0; i < (int)list.size() && i < MAX_LEADERBOARD_ENTRIES; ++i) {
            const auto& e = list[i];
            int ry = py + 74 + i * 24;

            // Highlight top 3
            uint8_t rowA = (i == 0) ? 50 : (i == 1) ? 30 : (i == 2) ? 20 : 0;
            if (rowA > 0) {
                SDL_SetRenderDrawColor(r, borCol.r/4, borCol.g/4, borCol.b/4, rowA);
                SDL_Rect hi = { px+4, ry-2, PANEL_W-8, 22 };
                SDL_RenderFillRect(r, &hi);
            }

            SDL_Color rc = (i == 0) ? SDL_Color{255, 200, 40, 255}
                         : (i == 1) ? SDL_Color{200, 200, 200, 255}
                         : (i == 2) ? SDL_Color{200, 130, 60, 255}
                         :            SDL_Color{120, 160, 140, 220};

            ctx.hud->drawLabel(std::to_string(i + 1),       px + 16,  ry, rc);
            ctx.hud->drawLabel(e.name,                       px + 42,  ry, rc);
            ctx.hud->drawLabel(std::to_string(e.score),      px + 220, ry, rc);
            if (endless)
                ctx.hud->drawLabel("W" + std::to_string(e.wave), px + 360, ry, rc);
        }
    };

    drawPanel(px0,              py0, false);
    drawPanel(px0 + PANEL_W + PANEL_PAD, py0, true);

    // Back prompt
    if (m_canExit) {
        float blink = 0.5f + 0.5f * std::sin(m_time * 3.f);
        SDL_Color pr = { 0, (uint8_t)(140 + 60 * blink), (uint8_t)(120 + 50 * blink), 220 };
        const char* prompt = ">> PRESS ANY KEY TO RETURN <<";
        int pw = ctx.hud->measureText(prompt);
        ctx.hud->drawLabel(prompt, Constants::SCREEN_W / 2 - pw / 2,
                           py0 + PANEL_H + 20, pr);
    }

    SDL_RenderPresent(r);
}
