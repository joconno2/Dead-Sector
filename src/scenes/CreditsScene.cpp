#include "CreditsScene.hpp"
#include "MainMenuScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "core/Constants.hpp"

#include <SDL.h>
#include <cmath>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Credits content
// ---------------------------------------------------------------------------
struct CreditsEntry {
    const char* text;
    bool        isHeader;   // headers are larger / brighter
};

static const CreditsEntry CREDITS[] = {
    { "DEAD SECTOR",                        true  },
    { "",                                   false },
    { "DESIGN  PROGRAMMING  ART",           false },
    { "Jim O'Connor",                       true  },
    { "",                                   false },
    { "",                                   false },
    { "MUSIC",                              false },
    { "Karl Casey",                         true  },
    { "White Bat Audio",                    false },
    { "Free for commercial use",            false },
    { "with attribution",                   false },
    { "whitebataudio.com",                  false },
    { "",                                   false },
    { "",                                   false },
    { "FONT",                               false },
    { "Share Tech Mono",                    true  },
    { "SIL Open Font License",              false },
    { "scripts.sil.org/OFL",               false },
    { "",                                   false },
    { "",                                   false },
    { "BUILT WITH",                         false },
    { "SDL2  SDL_ttf  SDL_mixer",           true  },
    { "C++17  CMake",                       false },
    { "",                                   false },
    { "",                                   false },
    { "THANK YOU FOR PLAYING",              true  },
    { "",                                   false },
    { "",                                   false },
    { "",                                   false },
    { "[ PRESS ANY KEY TO RETURN ]",        false },
};
static constexpr int CREDITS_COUNT = (int)(sizeof(CREDITS) / sizeof(CREDITS[0]));

// Pixels per credit entry
static constexpr float LINE_H     = 28.f;
static constexpr float SCROLL_SPD = 40.f;  // px/sec

static float totalHeight() {
    return CREDITS_COUNT * LINE_H + (float)Constants::SCREEN_H;
}

// ---------------------------------------------------------------------------

void CreditsScene::onEnter(SceneContext&) {
    m_scroll = 0.f;
    m_time   = 0.f;
    m_done   = false;
}

void CreditsScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    bool pressed = false;
    if (ev.type == SDL_KEYDOWN)
        pressed = true;
    else if (ev.type == SDL_CONTROLLERBUTTONDOWN)
        pressed = (ev.cbutton.button == SDL_CONTROLLER_BUTTON_A ||
                   ev.cbutton.button == SDL_CONTROLLER_BUTTON_START ||
                   ev.cbutton.button == SDL_CONTROLLER_BUTTON_B);

    if (pressed)
        ctx.scenes->replace(std::make_unique<MainMenuScene>());
}

void CreditsScene::update(float dt, SceneContext&) {
    m_time   += dt;
    m_scroll += SCROLL_SPD * dt;
    if (m_scroll >= totalHeight())
        m_scroll = 0.f;  // loop
}

void CreditsScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;
    if (!vr || !r) return;

    vr->clear();
    vr->drawGrid(10, 25, 40);
    vr->drawCRTOverlay();

    if (!ctx.hud) { SDL_RenderPresent(r); return; }

    float pulse = 0.5f + 0.5f * std::sin(m_time * 1.5f);

    // Fade bars at top and bottom
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < 80; ++i) {
        Uint8 a = (Uint8)((1.f - i / 80.f) * (1.f - i / 80.f) * 200.f);
        SDL_SetRenderDrawColor(r, 0, 0, 0, a);
        SDL_RenderDrawLine(r, 0, i, Constants::SCREEN_W, i);
        SDL_RenderDrawLine(r, 0, Constants::SCREEN_H - 1 - i,
                              Constants::SCREEN_W, Constants::SCREEN_H - 1 - i);
    }

    // Scroll all entries
    float startY = (float)Constants::SCREEN_H - m_scroll;

    for (int i = 0; i < CREDITS_COUNT; ++i) {
        float y = startY + i * LINE_H;
        if (y < -LINE_H || y > Constants::SCREEN_H + LINE_H) continue;

        const CreditsEntry& e = CREDITS[i];
        if (!e.text || e.text[0] == '\0') continue;

        int tw = ctx.hud->measureText(e.text);
        int cx = Constants::SCREEN_W / 2 - tw / 2;

        SDL_Color col;
        if (e.isHeader) {
            col = { (uint8_t)(0),
                    (uint8_t)(190 + 50 * pulse),
                    (uint8_t)(150 + 60 * pulse),
                    240 };
        } else {
            col = { 60, 140, 110, 190 };
        }

        ctx.hud->drawLabel(e.text, cx, (int)y, col);

        // Underline for headers
        if (e.isHeader) {
            int lx0 = cx - 4;
            int lx1 = cx + tw + 4;
            Uint8 la = (Uint8)(120 * pulse);
            SDL_SetRenderDrawColor(r, 0, (Uint8)(160 * pulse), (Uint8)(200 * pulse), la);
            SDL_RenderDrawLine(r, lx0, (int)y + 24, lx1, (int)y + 24);
        }
    }

    SDL_RenderPresent(r);
}
