#include "WorldSelectScene.hpp"
#include "DifficultyScene.hpp"
#include "MainMenuScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "core/Constants.hpp"
#include "core/Worlds.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "world/NodeMap.hpp"

#include <SDL.h>
#include <cmath>
#include <string>

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
static constexpr int CARD_W  = 340;
static constexpr int CARD_H  = 460;
static constexpr int CARD_GAP = 20;
static constexpr int CARDS_TOTAL = 3 * CARD_W + 2 * CARD_GAP;  // 1060
static constexpr int CARD_X0 = (Constants::SCREEN_W - CARDS_TOTAL) / 2;  // 110
static constexpr int CARD_Y  = (Constants::SCREEN_H - CARD_H) / 2;       // 130
static constexpr int ART_CY_OFFSET = 170; // art center y relative to card top

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void WorldSelectScene::onEnter(SceneContext& ctx) {
    m_time    = 0.f;
    m_pulse   = 0.f;
    m_cursor  = 0;
    m_unlocked = ctx.saveData ? ctx.saveData->worldsUnlocked : 1;
}

void WorldSelectScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    auto move = [&](int d) {
        int next = m_cursor + d;
        if (next < 0) next = 0;
        if (next >= worldCount()) next = worldCount() - 1;
        m_cursor = next;
    };

    if (ev.type == SDL_KEYDOWN) {
        const auto sc = ev.key.keysym.scancode;
        if      (sc == SDL_SCANCODE_LEFT  || sc == SDL_SCANCODE_A) move(-1);
        else if (sc == SDL_SCANCODE_RIGHT || sc == SDL_SCANCODE_D) move(+1);
        else if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_SPACE) confirm(ctx);
        else if (sc == SDL_SCANCODE_ESCAPE) ctx.scenes->replace(std::make_unique<MainMenuScene>());
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        const auto btn = ev.cbutton.button;
        if      (btn == SDL_CONTROLLER_BUTTON_DPAD_LEFT)  move(-1);
        else if (btn == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) move(+1);
        else if (btn == SDL_CONTROLLER_BUTTON_A || btn == SDL_CONTROLLER_BUTTON_START) confirm(ctx);
        else if (btn == SDL_CONTROLLER_BUTTON_B) ctx.scenes->replace(std::make_unique<MainMenuScene>());
    } else if (ev.type == SDL_MOUSEMOTION ||
               (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT)) {
        int mx = (ev.type == SDL_MOUSEMOTION) ? ev.motion.x : ev.button.x;
        int my = (ev.type == SDL_MOUSEMOTION) ? ev.motion.y : ev.button.y;
        for (int w = 0; w < worldCount(); ++w) {
            int cardX = CARD_X0 + w * (CARD_W + CARD_GAP);
            if (mx >= cardX && mx < cardX + CARD_W && my >= CARD_Y && my < CARD_Y + CARD_H) {
                m_cursor = w;
                if (ev.type == SDL_MOUSEBUTTONDOWN) confirm(ctx);
                break;
            }
        }
    }
}

void WorldSelectScene::confirm(SceneContext& ctx) {
    if (m_cursor >= m_unlocked) return;  // locked world
    ctx.currentWorld = m_cursor;
    if (ctx.nodeMap) ctx.nodeMap->reset(ctx.currentWorld);
    ctx.scenes->replace(std::make_unique<DifficultyScene>());
}

void WorldSelectScene::update(float dt, SceneContext&) {
    m_time  += dt;
    m_pulse += dt * 2.f;
}

// ---------------------------------------------------------------------------
// Vector art helpers
// ---------------------------------------------------------------------------

// World 0: circuit board — 3×3 grid of nodes with traces
void WorldSelectScene::drawCircuitArt(SDL_Renderer* r, float cx, float cy, float t) const {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    static const float NODE_SPACING = 36.f;
    static const int   GRID = 3;

    float offX = cx - (GRID - 1) * NODE_SPACING * 0.5f;
    float offY = cy - (GRID - 1) * NODE_SPACING * 0.5f;

    float pulse = 0.5f + 0.5f * std::sin(t * 2.f);

    // Horizontal and vertical traces
    SDL_SetRenderDrawColor(r, 0, (Uint8)(120 + 60 * pulse), (Uint8)(180 + 40 * pulse), 160);
    for (int row = 0; row < GRID; ++row) {
        float y = offY + row * NODE_SPACING;
        for (int col = 0; col < GRID - 1; ++col) {
            float x0 = offX + col * NODE_SPACING + 5;
            float x1 = offX + (col + 1) * NODE_SPACING - 5;
            SDL_RenderDrawLine(r, (int)x0, (int)y, (int)x1, (int)y);
        }
    }
    for (int col = 0; col < GRID; ++col) {
        float x = offX + col * NODE_SPACING;
        for (int row = 0; row < GRID - 1; ++row) {
            float y0 = offY + row * NODE_SPACING + 5;
            float y1 = offY + (row + 1) * NODE_SPACING - 5;
            SDL_RenderDrawLine(r, (int)x, (int)y0, (int)x, (int)y1);
        }
    }

    // Extra diagonal branch from center
    float midX = offX + NODE_SPACING;
    float midY = offY + NODE_SPACING;
    SDL_SetRenderDrawColor(r, 0, (Uint8)(80 + 40 * pulse), (Uint8)(200 + 40 * pulse), 100);
    SDL_RenderDrawLine(r, (int)midX, (int)midY, (int)(midX + 28), (int)(midY - 28));
    SDL_RenderDrawLine(r, (int)(midX + 28), (int)(midY - 28), (int)(midX + 50), (int)(midY - 28));

    // Node squares
    for (int row = 0; row < GRID; ++row) {
        for (int col = 0; col < GRID; ++col) {
            float nx = offX + col * NODE_SPACING;
            float ny = offY + row * NODE_SPACING;
            bool  isCenter = (row == 1 && col == 1);
            int   sz = isCenter ? 9 : 5;
            Uint8 bright = isCenter ? (Uint8)(200 + 55 * pulse) : 170;
            SDL_SetRenderDrawColor(r, 0, bright, (Uint8)(220 + 35 * pulse), 230);
            SDL_Rect nr = { (int)(nx - sz/2), (int)(ny - sz/2), sz, sz };
            SDL_RenderFillRect(r, &nr);
        }
    }
}

// World 1: neural hexagons with connecting web
void WorldSelectScene::drawNeuralArt(SDL_Renderer* r, float cx, float cy, float t) const {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    float pulse = 0.5f + 0.5f * std::sin(t * 2.5f);

    auto hexPt = [](float cx2, float cy2, float radius, int i, int sides) -> std::pair<float,float> {
        float a = (float)i / sides * 6.28318f - 1.5708f;
        return { cx2 + std::cos(a) * radius, cy2 + std::sin(a) * radius };
    };

    // Outer hex
    {
        SDL_SetRenderDrawColor(r, (Uint8)(140 + 60 * pulse), 40, (Uint8)(200 + 55 * pulse), 200);
        for (int i = 0; i < 6; ++i) {
            auto [x0, y0] = hexPt(cx, cy, 72.f, i,     6);
            auto [x1, y1] = hexPt(cx, cy, 72.f, i + 1, 6);
            SDL_RenderDrawLine(r, (int)x0, (int)y0, (int)x1, (int)y1);
        }
    }

    // Inner hex
    {
        Uint8 a2 = (Uint8)(140 + 80 * pulse);
        SDL_SetRenderDrawColor(r, (Uint8)(100 + 80 * pulse), 20, a2, 200);
        for (int i = 0; i < 6; ++i) {
            auto [x0, y0] = hexPt(cx, cy, 36.f, i,     6);
            auto [x1, y1] = hexPt(cx, cy, 36.f, i + 1, 6);
            SDL_RenderDrawLine(r, (int)x0, (int)y0, (int)x1, (int)y1);
        }
    }

    // Spokes: outer vertex → inner vertex
    SDL_SetRenderDrawColor(r, (Uint8)(80 + 60 * pulse), 20, (Uint8)(160 + 60 * pulse), 140);
    for (int i = 0; i < 6; ++i) {
        auto [ox, oy] = hexPt(cx, cy, 72.f, i, 6);
        auto [ix, iy] = hexPt(cx, cy, 36.f, i, 6);
        SDL_RenderDrawLine(r, (int)ox, (int)oy, (int)ix, (int)iy);
    }

    // Inner → center dots
    SDL_SetRenderDrawColor(r, (Uint8)(180 + 60 * pulse), 40, (Uint8)(240), 180);
    for (int i = 0; i < 6; ++i) {
        auto [ix, iy] = hexPt(cx, cy, 36.f, i, 6);
        SDL_RenderDrawLine(r, (int)ix, (int)iy, (int)cx, (int)cy);
    }

    // Centre dot
    SDL_SetRenderDrawColor(r, 220, 80, 255, (Uint8)(200 + 55 * pulse));
    SDL_Rect dot = { (int)cx - 4, (int)cy - 4, 8, 8 };
    SDL_RenderFillRect(r, &dot);
}

// World 2: reactor core — concentric rings + spokes
void WorldSelectScene::drawReactorArt(SDL_Renderer* r, float cx, float cy, float t) const {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    float pulse = 0.5f + 0.5f * std::sin(t * 3.f);
    float spin  = t * 0.6f;

    // Three concentric rings
    const float radii[] = { 70.f, 50.f, 28.f };
    const Uint8 alphas[] = { (Uint8)(120 + 60*pulse), (Uint8)(160 + 60*pulse), (Uint8)(200 + 55*pulse) };
    constexpr int SIDES = 32;
    for (int ring = 0; ring < 3; ++ring) {
        float rad = radii[ring];
        SDL_SetRenderDrawColor(r, (Uint8)(200 + 55*pulse), (Uint8)(60 + 40*pulse), 20, alphas[ring]);
        for (int i = 0; i < SIDES; ++i) {
            float a0 = (float)i       / SIDES * 6.28318f;
            float a1 = (float)(i + 1) / SIDES * 6.28318f;
            SDL_RenderDrawLine(r,
                (int)(cx + std::cos(a0) * rad), (int)(cy + std::sin(a0) * rad),
                (int)(cx + std::cos(a1) * rad), (int)(cy + std::sin(a1) * rad));
        }
    }

    // 8 spinning radial spokes from inner ring outward
    SDL_SetRenderDrawColor(r, 255, (Uint8)(100 + 60*pulse), 20, (Uint8)(120 + 80*pulse));
    for (int i = 0; i < 8; ++i) {
        float a = spin + (float)i / 8.f * 6.28318f;
        SDL_RenderDrawLine(r,
            (int)(cx + std::cos(a) * 28.f), (int)(cy + std::sin(a) * 28.f),
            (int)(cx + std::cos(a) * 70.f), (int)(cy + std::sin(a) * 70.f));
    }

    // Pulsing centre
    Uint8 cr = (Uint8)(230 + 25*pulse);
    Uint8 cg = (Uint8)(120 + 80*pulse);
    SDL_SetRenderDrawColor(r, cr, cg, 20, 255);
    int cs = (int)(5 + 4 * pulse);
    SDL_Rect dot = { (int)cx - cs, (int)cy - cs, cs*2, cs*2 };
    SDL_RenderFillRect(r, &dot);
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void WorldSelectScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;
    if (!vr || !r) return;

    const WorldDef& curDef = worldDef(m_cursor);
    vr->clear();
    vr->drawGrid(curDef.theme.gridR / 2, curDef.theme.gridG / 2, curDef.theme.gridB / 2);
    vr->drawCRTOverlay();
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    if (!ctx.hud) { SDL_RenderPresent(r); return; }

    // Title
    float titlePulse = 0.5f + 0.5f * std::sin(m_pulse * 0.9f);
    SDL_Color titleCol = { 0, (uint8_t)(180 + 60 * titlePulse), (uint8_t)(140 + 70 * titlePulse), 255 };
    {
        const char* title = "SELECT WORLD";
        ctx.hud->drawLabel(title, Constants::SCREEN_W / 2 - ctx.hud->measureText(title) / 2, 18, titleCol);
    }

    // Navigation hint
    {
        const char* hint = "< LEFT / RIGHT >  ENTER=SELECT  ESC=BACK";
        ctx.hud->drawLabel(hint,
                           Constants::SCREEN_W / 2 - ctx.hud->measureText(hint) / 2,
                           Constants::SCREEN_H - 26,
                           { 50, 90, 70, 150 });
    }

    // Cards
    for (int w = 0; w < worldCount(); ++w) {
        const WorldDef& wd = worldDef(w);
        bool selected = (w == m_cursor);
        bool locked   = (w >= m_unlocked);

        int cardX = CARD_X0 + w * (CARD_W + CARD_GAP);
        int artCY  = CARD_Y + ART_CY_OFFSET;

        float sp = selected ? (0.5f + 0.5f * std::sin(m_pulse * 2.f)) : 0.f;

        // Clip all rendering to this card — prevents text/art bleeding into adjacent cards
        SDL_Rect clipRect = { cardX, CARD_Y, CARD_W, CARD_H };
        SDL_RenderSetClipRect(r, &clipRect);

        // Card background
        Uint8 bgA = selected ? 180 : 100;
        SDL_SetRenderDrawColor(r, 4, 8, 16, bgA);
        SDL_Rect bg = { cardX, CARD_Y, CARD_W, CARD_H };
        SDL_RenderFillRect(r, &bg);

        // Card border
        if (selected) {
            SDL_SetRenderDrawColor(r, wd.theme.accentR,
                                      (Uint8)(wd.theme.accentG + (Uint8)(30*sp)),
                                      wd.theme.accentB, 220);
        } else {
            SDL_SetRenderDrawColor(r, 40, 60, 50, 140);
        }
        SDL_RenderDrawRect(r, &bg);

        // World number + name header
        SDL_Color hdrCol;
        if (locked) {
            hdrCol = { 50, 60, 50, 150 };
        } else if (selected) {
            hdrCol = { wd.theme.accentR,
                       (uint8_t)(wd.theme.accentG + (uint8_t)(30*sp)),
                       wd.theme.accentB, 255 };
        } else {
            hdrCol = { (uint8_t)(wd.theme.accentR * 0.5f),
                       (uint8_t)(wd.theme.accentG * 0.5f),
                       (uint8_t)(wd.theme.accentB * 0.5f), 200 };
        }
        std::string worldNum = "WORLD " + std::to_string(w + 1);
        ctx.hud->drawLabel(worldNum.c_str(), cardX + 12, CARD_Y + 12, hdrCol);
        ctx.hud->drawLabel(wd.name,          cardX + 12, CARD_Y + 36, hdrCol);

        SDL_SetRenderDrawColor(r, hdrCol.r / 2, hdrCol.g / 2, hdrCol.b / 2, 100);
        SDL_RenderDrawLine(r, cardX + 8, CARD_Y + 60, cardX + CARD_W - 8, CARD_Y + 60);

        // Art area separator
        SDL_SetRenderDrawColor(r, hdrCol.r / 2, hdrCol.g / 2, hdrCol.b / 2, 80);
        SDL_RenderDrawLine(r, cardX + 8, CARD_Y + 320, cardX + CARD_W - 8, CARD_Y + 320);

        if (locked) {
            // Lock indicator
            ctx.hud->drawLabel("[ LOCKED ]",
                               cardX + CARD_W / 2 - 36, CARD_Y + ART_CY_OFFSET - 8,
                               { 60, 70, 60, 180 });
            ctx.hud->drawLabel("BEAT PREVIOUS WORLD",
                               cardX + 12, CARD_Y + ART_CY_OFFSET + 20,
                               { 50, 60, 50, 140 });
        } else {
            // Draw world-specific art
            float artCX = cardX + CARD_W * 0.5f;
            float artCYf = (float)artCY;
            switch (w) {
                case 0: drawCircuitArt(r, artCX, artCYf, m_time); break;
                case 1: drawNeuralArt (r, artCX, artCYf, m_time); break;
                case 2: drawReactorArt(r, artCX, artCYf, m_time); break;
            }
        }

        // Subtitle + flavor (below art)
        SDL_Color subCol = locked ? SDL_Color{40,50,40,120} : SDL_Color{100,140,110,190};
        SDL_Color flvCol = locked ? SDL_Color{40,50,40,100} : SDL_Color{70, 100, 80, 150};
        ctx.hud->drawLabel(wd.subtitle, cardX + 12, CARD_Y + 330, subCol);
        // Flavor text word-wrapped to fit the card width
        ctx.hud->drawWrapped(wd.flavor, cardX + 12, CARD_Y + 354, CARD_W - 24, flvCol, 22);

        // Difficulty dots — two rows from bottom so PRESS ENTER fits below
        if (!locked) {
            int dots = w + 1;
            SDL_Color dotCol = { wd.theme.accentR, wd.theme.accentG, wd.theme.accentB,
                                 (uint8_t)(selected ? 220 : 150) };
            std::string dotStr;
            for (int d = 0; d < dots; ++d) dotStr += "* ";
            for (int d = dots; d < 3;  ++d) dotStr += "- ";
            ctx.hud->drawLabel(("DIFFICULTY: " + dotStr).c_str(),
                               cardX + 12, CARD_Y + CARD_H - 56, dotCol);
        }

        // Cursor indicator — 30px from bottom, below difficulty row
        if (selected && !locked) {
            SDL_Color promptCol = { wd.theme.accentR,
                                    (uint8_t)(wd.theme.accentG + (uint8_t)(40*sp)),
                                    wd.theme.accentB, (uint8_t)(180 + 60*sp) };
            ctx.hud->drawLabel("[ PRESS ENTER ]",
                               cardX + CARD_W / 2 - 56, CARD_Y + CARD_H - 30, promptCol);
        }

        // Restore clip rect after each card
        SDL_RenderSetClipRect(r, nullptr);
    }

    SDL_RenderPresent(r);
}
