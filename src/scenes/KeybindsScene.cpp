#include "KeybindsScene.hpp"
#include "SettingsScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/Constants.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"

#include <SDL.h>
#include <cmath>
#include <string>

// Scancodes that must not be rebound (reserved for menus / system)
static bool isReservedKey(SDL_Scancode sc) {
    return sc == SDL_SCANCODE_ESCAPE
        || sc == SDL_SCANCODE_UP    || sc == SDL_SCANCODE_DOWN
        || sc == SDL_SCANCODE_LEFT  || sc == SDL_SCANCODE_RIGHT
        || sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_KP_ENTER
        || sc == SDL_SCANCODE_UNKNOWN;
}

// Controller buttons that must not be rebound (START is always menu/back)
static bool isReservedButton(SDL_GameControllerButton b) {
    return b == SDL_CONTROLLER_BUTTON_START
        || b == SDL_CONTROLLER_BUTTON_INVALID;
}

void KeybindsScene::onEnter(SceneContext& ctx) {
    m_time      = 0.f;
    m_cursor    = 0;
    m_col       = 0;
    m_listening = false;
    m_bindings  = ctx.saveData ? ctx.saveData->bindings : Bindings{};
}

void KeybindsScene::save(SceneContext& ctx) {
    if (ctx.saveData) {
        ctx.saveData->bindings = m_bindings;
        SaveSystem::save(*ctx.saveData);
    }
}

void KeybindsScene::goBack(SceneContext& ctx) {
    save(ctx);
    ctx.scenes->replace(std::make_unique<SettingsScene>());
}

void KeybindsScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    if (m_listening) {
        if (m_col == 0) {
            // Listening for keyboard key
            if (ev.type != SDL_KEYDOWN) return;
            SDL_Scancode sc = ev.key.keysym.scancode;
            if (sc == SDL_SCANCODE_ESCAPE) { m_listening = false; return; }
            if (isReservedKey(sc)) return;
            m_bindings.*BINDING_DEFS[m_cursor].kbField = sc;
            m_listening = false;
            save(ctx);
        } else {
            // Listening for controller button
            if (ev.type != SDL_CONTROLLERBUTTONDOWN) return;
            SDL_GameControllerButton b = (SDL_GameControllerButton)ev.cbutton.button;
            if (b == SDL_CONTROLLER_BUTTON_START) { m_listening = false; return; } // START = cancel
            if (isReservedButton(b)) return;
            m_bindings.*BINDING_DEFS[m_cursor].ctlField = b;
            m_listening = false;
            save(ctx);
        }
        return;
    }

    // Normal navigation
    if (ev.type == SDL_KEYDOWN) {
        switch (ev.key.keysym.scancode) {
        case SDL_SCANCODE_UP:   case SDL_SCANCODE_W:
            m_cursor = (m_cursor - 1 + BINDING_COUNT) % BINDING_COUNT;
            break;
        case SDL_SCANCODE_DOWN: case SDL_SCANCODE_S:
            m_cursor = (m_cursor + 1) % BINDING_COUNT;
            break;
        case SDL_SCANCODE_LEFT:  case SDL_SCANCODE_A:
            m_col = 0;
            break;
        case SDL_SCANCODE_RIGHT: case SDL_SCANCODE_D:
            m_col = 1;
            break;
        case SDL_SCANCODE_RETURN: case SDL_SCANCODE_SPACE:
            m_listening = true;
            break;
        case SDL_SCANCODE_ESCAPE:
            goBack(ctx);
            break;
        case SDL_SCANCODE_R:
            m_bindings = Bindings{};
            save(ctx);
            break;
        default: break;
        }
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (ev.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:   m_cursor = (m_cursor - 1 + BINDING_COUNT) % BINDING_COUNT; break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: m_cursor = (m_cursor + 1) % BINDING_COUNT; break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  m_col = 0; break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: m_col = 1; break;
        case SDL_CONTROLLER_BUTTON_A:         m_listening = true; break;
        case SDL_CONTROLLER_BUTTON_B:
        case SDL_CONTROLLER_BUTTON_START:     goBack(ctx); break;
        default: break;
        }
    }
}

void KeybindsScene::update(float dt, SceneContext&) {
    m_time += dt;
}

void KeybindsScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;

    vr->clear();
    vr->drawGrid(0, 25, 40);
    vr->drawCRTOverlay();

    if (!ctx.hud) { SDL_RenderPresent(r); return; }

    constexpr int PANEL_W = 720;
    constexpr int PANEL_H = 400;
    int panelX = Constants::SCREEN_W / 2 - PANEL_W / 2;
    int panelY = Constants::SCREEN_H / 2 - PANEL_H / 2;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 10, 18, 220);
    SDL_Rect bg = { panelX, panelY, PANEL_W, PANEL_H };
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawColor(r, 0, 160, 220, 200);
    SDL_RenderDrawRect(r, &bg);

    ctx.hud->drawLabel("// KEY BINDINGS //", panelX + 16, panelY + 14, {0, 200, 240, 255});
    SDL_SetRenderDrawColor(r, 0, 80, 120, 140);
    SDL_RenderDrawLine(r, panelX+12, panelY+46, panelX+PANEL_W-12, panelY+46);

    // Column positions
    constexpr int COL_ACTION = 20;
    constexpr int COL_KB     = 240;
    constexpr int COL_CTL    = 450;

    // Column headers
    SDL_Color hdrNorm  = { 60, 130, 160, 200 };
    SDL_Color hdrSelKB = { 0, 200, 240, 255 };
    SDL_Color hdrSelCtl = { 0, 200, 240, 255 };
    ctx.hud->drawLabel("ACTION",     panelX + COL_ACTION, panelY + 54, hdrNorm);
    ctx.hud->drawLabel("KEYBOARD",   panelX + COL_KB,     panelY + 54, m_col == 0 ? hdrSelKB  : hdrNorm);
    ctx.hud->drawLabel("CONTROLLER", panelX + COL_CTL,    panelY + 54, m_col == 1 ? hdrSelCtl : hdrNorm);

    // Underline selected column header
    SDL_SetRenderDrawColor(r, 0, 160, 220, 180);
    if (m_col == 0) {
        int x = panelX + COL_KB;
        SDL_RenderDrawLine(r, x, panelY+68, x + 90, panelY+68);
    } else {
        int x = panelX + COL_CTL;
        SDL_RenderDrawLine(r, x, panelY+68, x + 110, panelY+68);
    }

    SDL_SetRenderDrawColor(r, 0, 60, 90, 120);
    SDL_RenderDrawLine(r, panelX+12, panelY+74, panelX+PANEL_W-12, panelY+74);

    for (int i = 0; i < BINDING_COUNT; ++i) {
        int ry = panelY + 82 + i * 34;
        bool sel = (m_cursor == i);
        float pulse = sel ? (0.5f + 0.5f * std::sin(m_time * 4.f)) : 0.f;

        SDL_Color selCol  = { 0, (Uint8)(180 + 60*pulse), (Uint8)(220 + 35*pulse), 255 };
        SDL_Color normCol = { 60, 120, 150, 200 };

        if (sel) {
            SDL_SetRenderDrawColor(r, 0, (Uint8)(30+20*pulse), (Uint8)(50+15*pulse), 120);
            SDL_Rect hi = { panelX+8, ry-3, PANEL_W-16, 28 };
            SDL_RenderFillRect(r, &hi);
        }

        SDL_Color labelCol = sel ? selCol : normCol;
        std::string rowLabel = sel ? ("> " + std::string(BINDING_DEFS[i].label))
                                   : ("  " + std::string(BINDING_DEFS[i].label));
        ctx.hud->drawLabel(rowLabel, panelX + COL_ACTION, ry, labelCol);

        // Keyboard column
        std::string kbName;
        if (m_listening && sel && m_col == 0) {
            bool blink = (int)(m_time * 2.f) % 2 == 0;
            kbName = blink ? "[ PRESS KEY ]" : "             ";
        } else {
            kbName = SDL_GetScancodeName(m_bindings.*BINDING_DEFS[i].kbField);
        }
        SDL_Color kbCol = (m_listening && sel && m_col == 0)
            ? SDL_Color{ 255, 200, 40, 255 }
            : (sel && m_col == 0 ? selCol : SDL_Color{ 180, 220, 240, 230 });
        ctx.hud->drawLabel(kbName, panelX + COL_KB, ry, kbCol);

        // Controller column
        std::string ctlName;
        if (m_listening && sel && m_col == 1) {
            bool blink = (int)(m_time * 2.f) % 2 == 0;
            ctlName = blink ? "[ PRESS BTN ]" : "             ";
        } else {
            ctlName = ctlButtonName(m_bindings.*BINDING_DEFS[i].ctlField);
        }
        SDL_Color ctlCol = (m_listening && sel && m_col == 1)
            ? SDL_Color{ 255, 200, 40, 255 }
            : (sel && m_col == 1 ? selCol : SDL_Color{ 180, 220, 240, 230 });
        ctx.hud->drawLabel(ctlName, panelX + COL_CTL, ry, ctlCol);
    }

    // Separator + bottom hints
    SDL_SetRenderDrawColor(r, 0, 60, 90, 120);
    SDL_RenderDrawLine(r, panelX+12, panelY+PANEL_H-50, panelX+PANEL_W-12, panelY+PANEL_H-50);

    SDL_Color hint = { 50, 100, 130, 180 };
    const char* hintTxt;
    if (m_listening && m_col == 0)
        hintTxt = "Press a key to bind   ESC: cancel";
    else if (m_listening && m_col == 1)
        hintTxt = "Press a button to bind   START: cancel";
    else
        hintTxt = "L/R: column   ENTER: rebind   R: reset   ESC: back";
    int hw = ctx.hud->measureText(hintTxt);
    ctx.hud->drawLabel(hintTxt, panelX + PANEL_W/2 - hw/2, panelY + PANEL_H - 38, hint);

    SDL_Color note = { 40, 80, 100, 140 };
    const char* noteTxt = "Arrow keys / DPAD / Right Trigger always active as fallbacks";
    int nw = ctx.hud->measureText(noteTxt);
    ctx.hud->drawLabel(noteTxt, panelX + PANEL_W/2 - nw/2, panelY + PANEL_H - 20, note);

    SDL_RenderPresent(r);
}
