#include "DifficultyScene.hpp"
#include "ShipSelectScene.hpp"
#include "WorldSelectScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "core/Constants.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"

#include <SDL.h>
#include <cmath>
#include <string>

// ---------------------------------------------------------------------------
// Difficulty definitions
// ---------------------------------------------------------------------------
struct DiffDef {
    const char* name;
    const char* subtitle;
    const char* desc[3];   // 3 lines of descriptor text
    float       traceRate; // %/sec trace tick multiplier relative to baseline
    SDL_Color   color;
};

static const DiffDef DIFFS[3] = {
    {
        "RUNNER",
        "// NOVICE COWBOY //",
        { "Trace ticks slowly.", "ICE responds later.", "For learning the grid." },
        0.65f,
        { 80, 220, 120, 255 }
    },
    {
        "DECKER",
        "// STANDARD RUN //",
        { "Baseline trace rate.", "Recommended for all.", "The grid is hungry." },
        1.0f,
        { 80, 180, 255, 255 }
    },
    {
        "NETRUNNER",
        "// FLATLINE RISK //",
        { "Trace surges fast.", "ICE spawns aggressive.", "Only the elite survive." },
        1.6f,
        { 255, 60, 80, 255 }
    }
};

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void DifficultyScene::onEnter(SceneContext& ctx) {
    m_cursor = ctx.difficulty;  // restore last selected
    m_pulse  = 0.f;
    m_time   = 0.f;
}

void DifficultyScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    auto move = [&](int d) {
        m_cursor = std::max(0, std::min(2, m_cursor + d));
    };

    if (ev.type == SDL_KEYDOWN) {
        const auto sc = ev.key.keysym.scancode;
        if      (sc == SDL_SCANCODE_LEFT  || sc == SDL_SCANCODE_A) move(-1);
        else if (sc == SDL_SCANCODE_RIGHT || sc == SDL_SCANCODE_D) move(+1);
        else if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_SPACE) confirm(ctx);
        else if (sc == SDL_SCANCODE_ESCAPE) ctx.scenes->replace(std::make_unique<WorldSelectScene>());
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        const auto btn = ev.cbutton.button;
        if      (btn == SDL_CONTROLLER_BUTTON_DPAD_LEFT)  move(-1);
        else if (btn == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) move(+1);
        else if (btn == SDL_CONTROLLER_BUTTON_A || btn == SDL_CONTROLLER_BUTTON_START) confirm(ctx);
        else if (btn == SDL_CONTROLLER_BUTTON_B) ctx.scenes->replace(std::make_unique<WorldSelectScene>());
    } else if (ev.type == SDL_MOUSEMOTION ||
               (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT)) {
        int mx = (ev.type == SDL_MOUSEMOTION) ? ev.motion.x : ev.button.x;
        int my = (ev.type == SDL_MOUSEMOTION) ? ev.motion.y : ev.button.y;
        constexpr int CW = 320, CH = 260, CGAP = 28;
        constexpr int CTOTAL = 3 * CW + 2 * CGAP;
        int cx0 = (Constants::SCREEN_W - CTOTAL) / 2;
        int cy0 = (Constants::SCREEN_H - CH) / 2 - 20;
        for (int i = 0; i < 3; ++i) {
            int cx = cx0 + i * (CW + CGAP);
            if (mx >= cx && mx < cx + CW && my >= cy0 && my < cy0 + CH) {
                m_cursor = i;
                if (ev.type == SDL_MOUSEBUTTONDOWN) confirm(ctx);
                break;
            }
        }
    }
}

void DifficultyScene::confirm(SceneContext& ctx) {
    ctx.difficulty = m_cursor;
    ctx.scenes->replace(std::make_unique<ShipSelectScene>());
}

void DifficultyScene::update(float dt, SceneContext&) {
    m_time  += dt;
    m_pulse += dt * 2.5f;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void DifficultyScene::render(SceneContext& ctx) {
    ctx.vrenderer->clear();
    ctx.vrenderer->drawGrid(15, 30, 50);
    ctx.vrenderer->drawCRTOverlay();

    SDL_Renderer* r = ctx.renderer;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    if (!ctx.hud) { SDL_RenderPresent(r); return; }

    // Header
    SDL_Color headerCol = { 0, 220, 180, 255 };
    ctx.hud->drawLabel("// SELECT DIFFICULTY //",
        Constants::SCREEN_W / 2 - 170, 28, headerCol);

    // Cards
    constexpr int CW     = 320;
    constexpr int CH     = 260;
    constexpr int CGAP   = 28;
    constexpr int CTOTAL = 3 * CW + 2 * CGAP;
    int cx0 = (Constants::SCREEN_W - CTOTAL) / 2;
    int cy0 = (Constants::SCREEN_H - CH) / 2 - 20;

    float pulse = 0.5f + 0.5f * std::sin(m_pulse);

    for (int i = 0; i < 3; ++i) {
        const DiffDef& d  = DIFFS[i];
        bool           sel = (i == m_cursor);
        int cx = cx0 + i * (CW + CGAP);
        int cy = cy0;

        // Card background
        uint8_t bgA = sel ? 200 : 110;
        SDL_SetRenderDrawColor(r, d.color.r / 10, d.color.g / 10, d.color.b / 10, bgA);
        SDL_Rect card = { cx, cy, CW, CH };
        SDL_RenderFillRect(r, &card);

        // Border
        if (sel) {
            uint8_t bA = (uint8_t)(160 + 95 * pulse);
            SDL_SetRenderDrawColor(r, d.color.r, d.color.g, d.color.b, bA);
            SDL_RenderDrawRect(r, &card);
            SDL_Rect g2 = { cx-2, cy-2, CW+4, CH+4 };
            SDL_SetRenderDrawColor(r, d.color.r, d.color.g, d.color.b, (uint8_t)(40 * pulse));
            SDL_RenderDrawRect(r, &g2);
        } else {
            SDL_SetRenderDrawColor(r, d.color.r / 2, d.color.g / 2, d.color.b / 2, 160);
            SDL_RenderDrawRect(r, &card);
        }

        // Accent stripe
        SDL_SetRenderDrawColor(r, d.color.r, d.color.g, d.color.b, sel ? 200 : 70);
        SDL_RenderDrawLine(r, cx+6, cy+5, cx+CW-6, cy+5);
        SDL_RenderDrawLine(r, cx+6, cy+6, cx+CW-6, cy+6);

        // Name
        uint8_t nA = sel ? 255 : 180;
        SDL_Color nameCol = { d.color.r, d.color.g, d.color.b, nA };
        SDL_Color subCol  = { (uint8_t)(d.color.r * 0.6f), (uint8_t)(d.color.g * 0.6f), (uint8_t)(d.color.b * 0.6f), nA };
        SDL_Color descCol = { 140, 165, 155, 200 };

        ctx.hud->drawLabel(d.name,     cx + 12, cy + 16, nameCol);
        ctx.hud->drawLabel(d.subtitle, cx + 12, cy + 42, subCol);

        // Separator line
        SDL_SetRenderDrawColor(r, 0, 60, 45, 100);
        SDL_RenderDrawLine(r, cx+10, cy+68, cx+CW-10, cy+68);

        // Description lines
        for (int ln = 0; ln < 3; ++ln)
            ctx.hud->drawLabel(d.desc[ln], cx + 14, cy + 82 + ln * 24, descCol);

        // Trace rate indicator
        std::string trStr = "TRACE RATE: ";
        if (i == 0) trStr += "LOW";
        else if (i == 1) trStr += "NORMAL";
        else trStr += "HIGH";
        SDL_Color trCol = { d.color.r, d.color.g, d.color.b, (uint8_t)(sel ? 220 : 140) };
        ctx.hud->drawLabel(trStr, cx + 14, cy + CH - 46, trCol);

        // Select prompt
        if (sel) {
            SDL_Color hint = { d.color.r, d.color.g, d.color.b, 210 };
            ctx.hud->drawLabel("[ ENTER ]", cx + 14, cy + CH - 26, hint);
        }
    }

    // Nav hint
    SDL_Color hint = { 60, 100, 80, 160 };
    std::string navHint = "LEFT / RIGHT: choose   ENTER: confirm   ESC: back";
    int hintW = ctx.hud->measureText(navHint);
    ctx.hud->drawLabel(navHint, Constants::SCREEN_W / 2 - hintW / 2,
                       Constants::SCREEN_H - 34, hint);

    SDL_RenderPresent(r);
}
