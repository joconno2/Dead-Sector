#include "SettingsScene.hpp"
#include "KeybindsScene.hpp"
#include "MainMenuScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/Constants.hpp"
#include "core/DisplaySettings.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "audio/AudioSystem.hpp"

#include <SDL.h>
#include <SDL_mixer.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string volBar(int vol) {
    int filled = vol / 10;
    std::string bar = "[";
    for (int i = 0; i < 10; ++i) bar += (i < filled ? '=' : ' ');
    bar += "] ";
    bar += std::to_string(vol) + "%";
    return bar;
}

static const char* dispModeName(int mode) {
    switch (mode) {
        case DISP_WINDOWED:    return "WINDOWED";
        case DISP_FULLSCREEN:  return "FULLSCREEN";
        case DISP_BORDERLESS:  return "BORDERLESS";
    }
    return "WINDOWED";
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void SettingsScene::onEnter(SceneContext& ctx) {
    m_time        = 0.f;
    m_cursor      = Row::DisplayMode;
    m_musicVol    = ctx.saveData ? ctx.saveData->musicVolume   : 80;
    m_sfxVol      = ctx.saveData ? ctx.saveData->sfxVolume     : 80;
    m_displayMode = ctx.saveData ? ctx.saveData->displayMode   : DISP_WINDOWED;
    int w         = ctx.saveData ? ctx.saveData->windowedWidth  : 1280;
    int h         = ctx.saveData ? ctx.saveData->windowedHeight : 720;
    m_resIdx      = findResolutionIndex(w, h);
    m_callsignBuf = ctx.saveData ? ctx.saveData->playerName : "RUNNER";
    m_editingCallsign = false;
}

void SettingsScene::applyVolumes(SceneContext& ctx) const {
    if (ctx.audio) {
        ctx.audio->setMusicVolume(m_musicVol * MIX_MAX_VOLUME / 100);
        ctx.audio->setSfxVolume(  m_sfxVol   * MIX_MAX_VOLUME / 100);
    }
}

void SettingsScene::applyDisplay(SceneContext& ctx) const {
    if (!ctx.window || !ctx.renderer) return;
    int w = RESOLUTIONS[m_resIdx].w;
    int h = RESOLUTIONS[m_resIdx].h;
    applyDisplaySettings(ctx.window, ctx.renderer, m_displayMode, w, h);
}

void SettingsScene::save(SceneContext& ctx) const {
    if (!ctx.saveData) return;
    ctx.saveData->musicVolume   = m_musicVol;
    ctx.saveData->sfxVolume     = m_sfxVol;
    ctx.saveData->displayMode   = m_displayMode;
    ctx.saveData->windowedWidth  = RESOLUTIONS[m_resIdx].w;
    ctx.saveData->windowedHeight = RESOLUTIONS[m_resIdx].h;
    SaveSystem::save(*ctx.saveData);
}

void SettingsScene::commitCallsign(SceneContext& ctx) {
    if (m_callsignBuf.empty()) m_callsignBuf = "RUNNER";
    if (ctx.saveData) {
        ctx.saveData->playerName = m_callsignBuf;
        SaveSystem::save(*ctx.saveData);
    }
    SDL_StopTextInput();
    m_editingCallsign = false;
}

void SettingsScene::goBack(SceneContext& ctx) {
    if (m_editingCallsign) { commitCallsign(ctx); return; }
    save(ctx);
    ctx.scenes->replace(std::make_unique<MainMenuScene>());
}

void SettingsScene::changeValue(int delta, SceneContext& ctx) {
    switch (m_cursor) {
        case Row::DisplayMode:
            m_displayMode = (m_displayMode + delta + 3) % 3;
            applyDisplay(ctx);
            break;
        case Row::Resolution:
            m_resIdx = (m_resIdx + delta + RESOLUTION_COUNT) % RESOLUTION_COUNT;
            if (m_displayMode != DISP_BORDERLESS) applyDisplay(ctx);
            break;
        case Row::Music:
            m_musicVol = std::max(0, std::min(100, m_musicVol + delta * 10));
            applyVolumes(ctx);
            break;
        case Row::SFX:
            m_sfxVol = std::max(0, std::min(100, m_sfxVol + delta * 10));
            applyVolumes(ctx);
            break;
        default: break;
    }
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void SettingsScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    // --- Text input mode for callsign ---
    if (m_editingCallsign) {
        if (ev.type == SDL_TEXTINPUT) {
            for (char c : std::string(ev.text.text)) {
                if ((int)m_callsignBuf.size() < 12 &&
                    (std::isalnum((unsigned char)c) || c == '_' || c == '-'))
                    m_callsignBuf += (char)std::toupper((unsigned char)c);
            }
            return;
        }
        if (ev.type == SDL_KEYDOWN) {
            switch (ev.key.keysym.sym) {
            case SDLK_BACKSPACE:
                if (!m_callsignBuf.empty()) m_callsignBuf.pop_back();
                break;
            case SDLK_RETURN: case SDLK_KP_ENTER:
                commitCallsign(ctx);
                break;
            case SDLK_ESCAPE:
                m_callsignBuf = ctx.saveData ? ctx.saveData->playerName : "RUNNER";
                SDL_StopTextInput();
                m_editingCallsign = false;
                break;
            default: break;
            }
        }
        return;
    }

    bool up = false, down = false, left = false, right = false;
    bool confirm = false, back = false;

    if (ev.type == SDL_KEYDOWN) {
        switch (ev.key.keysym.sym) {
        case SDLK_UP:    case SDLK_w: up      = true; break;
        case SDLK_DOWN:  case SDLK_s: down    = true; break;
        case SDLK_LEFT:  case SDLK_a: left    = true; break;
        case SDLK_RIGHT: case SDLK_d: right   = true; break;
        case SDLK_RETURN: case SDLK_SPACE: confirm = true; break;
        case SDLK_ESCAPE: back = true; break;
        default: break;
        }
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (ev.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:    up      = true; break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  down    = true; break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  left    = true; break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: right   = true; break;
        case SDL_CONTROLLER_BUTTON_A:          confirm = true; break;
        case SDL_CONTROLLER_BUTTON_B:
        case SDL_CONTROLLER_BUTTON_START:      back    = true; break;
        default: break;
        }
    } else if (ev.type == SDL_MOUSEMOTION ||
               (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT)) {
        int mx = (ev.type == SDL_MOUSEMOTION) ? ev.motion.x : ev.button.x;
        int my = (ev.type == SDL_MOUSEMOTION) ? ev.motion.y : ev.button.y;
        constexpr int panelW = 640;
        int panelX = Constants::SCREEN_W / 2 - panelW / 2;
        int panelY = Constants::SCREEN_H / 2 - 210;
        int ry = panelY + 58;
        for (int i = 0; i < (int)Row::COUNT; ++i) {
            if (mx >= panelX + 8 && mx < panelX + panelW - 8 && my >= ry - 3 && my < ry + 23) {
                m_cursor = (Row)i;
                if (ev.type == SDL_MOUSEBUTTONDOWN) confirm = true;
                break;
            }
            ry += 42;
        }
    }

    int rows = (int)Row::COUNT;
    if (up)    m_cursor = (Row)(((int)m_cursor - 1 + rows) % rows);
    if (down)  m_cursor = (Row)(((int)m_cursor + 1) % rows);
    if (left)  changeValue(-1, ctx);
    if (right) changeValue(+1, ctx);
    if (confirm) {
        if (m_cursor == Row::Back) {
            goBack(ctx);
        } else if (m_cursor == Row::Callsign) {
            m_callsignBuf = ctx.saveData ? ctx.saveData->playerName : "RUNNER";
            SDL_StartTextInput();
            m_editingCallsign = true;
        } else if (m_cursor == Row::KeybindsInfo) {
            save(ctx);
            ctx.scenes->replace(std::make_unique<KeybindsScene>());
        }
    }
    if (back) goBack(ctx);
}

void SettingsScene::update(float dt, SceneContext&) {
    m_time += dt;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void SettingsScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;
    if (!vr || !r) return;

    vr->clear();
    vr->drawGrid(0, 30, 15);
    vr->drawCRTOverlay();

    if (!ctx.hud) { SDL_RenderPresent(r); return; }

    int panelW = 640, panelH = 420;
    int panelX = Constants::SCREEN_W / 2 - panelW / 2;
    int panelY = Constants::SCREEN_H / 2 - panelH / 2;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 14, 10, 220);
    SDL_Rect bg = { panelX, panelY, panelW, panelH };
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawColor(r, 0, 180, 120, 200);
    SDL_RenderDrawRect(r, &bg);

    SDL_Color titleCol = { 0, 220, 150, 255 };
    ctx.hud->drawLabel("// SETTINGS //", panelX + 16, panelY + 14, titleCol);
    SDL_SetRenderDrawColor(r, 0, 100, 70, 160);
    SDL_RenderDrawLine(r, panelX+12, panelY+46, panelX+panelW-12, panelY+46);

    // Callsign display (with blinking cursor when editing)
    std::string callsignDisplay;
    if (m_editingCallsign) {
        bool blink = (int)(m_time * 2.f) % 2 == 0;
        callsignDisplay = m_callsignBuf + (blink ? "_" : " ");
    } else {
        callsignDisplay = ctx.saveData ? ctx.saveData->playerName : "RUNNER";
    }

    struct Entry { const char* label; std::string value; bool separator; };
    std::string resStr = (m_displayMode == DISP_BORDERLESS)
                         ? "NATIVE DESKTOP"
                         : RESOLUTIONS[m_resIdx].label;

    Entry entries[] = {
        { "DISPLAY MODE",  dispModeName(m_displayMode), false },
        { "RESOLUTION",    resStr,                      false },
        { "MUSIC VOLUME",  volBar(m_musicVol),           true  },
        { "SFX VOLUME",    volBar(m_sfxVol),             false },
        { "CALLSIGN",      callsignDisplay,              true  },
        { "KEYBINDS",      "CONFIGURE >>",               false },
        { "BACK",          "",                           false },
    };

    int ry = panelY + 58;
    for (int i = 0; i < (int)Row::COUNT; ++i) {
        bool sel    = (m_cursor == (Row)i);
        bool isCS   = ((Row)i == Row::Callsign);
        float pulse = sel ? (0.5f + 0.5f * std::sin(m_time * 4.f)) : 0.f;
        SDL_Color selCol  = { 0, (Uint8)(200 + 40*pulse), (Uint8)(160 + 60*pulse), 255 };
        SDL_Color normCol = { 80, 140, 120, 200 };
        SDL_Color valCol  = (isCS && m_editingCallsign)
                            ? SDL_Color{ 255, 220, 60, 255 }
                            : SDL_Color{ 180, 220, 100, 230 };
        SDL_Color dimVal  = { 120, 150, 110, 180 };

        if (entries[i].separator) {
            SDL_SetRenderDrawColor(r, 0, 80, 60, 100);
            SDL_RenderDrawLine(r, panelX+12, ry-8, panelX+panelW-12, ry-8);
        }

        if (sel) {
            SDL_SetRenderDrawColor(r, 0, (Uint8)(40+20*pulse), (Uint8)(30+15*pulse), 120);
            SDL_Rect row = { panelX+8, ry-3, panelW-16, 26 };
            SDL_RenderFillRect(r, &row);
        }

        SDL_Color labelCol = sel ? selCol : normCol;
        std::string labelTxt = sel ? ("> " + std::string(entries[i].label))
                                   : ("  " + std::string(entries[i].label));
        ctx.hud->drawLabel(labelTxt.c_str(), panelX+16, ry, labelCol);

        if (!entries[i].value.empty()) {
            SDL_Color vCol = (i == (int)Row::Resolution && m_displayMode == DISP_BORDERLESS)
                             ? dimVal : valCol;
            ctx.hud->drawLabel(entries[i].value.c_str(), panelX+260, ry, vCol);
        }

        if (isCS && sel && !m_editingCallsign) {
            SDL_Color hint = { 60, 140, 100, 160 };
            ctx.hud->drawLabel("ENTER to edit", panelX+420, ry, hint);
        }
        if ((Row)i == Row::KeybindsInfo && sel) {
            SDL_Color hint = { 60, 140, 100, 160 };
            ctx.hud->drawLabel("ENTER to open", panelX+420, ry, hint);
        }

        ry += 42;
    }

    SDL_Color hintCol = { 60, 100, 80, 160 };
    const char* hint = m_editingCallsign
        ? "A-Z 0-9 _ -   ENTER: confirm   ESC: cancel"
        : "L/R: adjust   UP/DN: navigate   ESC: back";
    int hintW = ctx.hud->measureText(hint);
    ctx.hud->drawLabel(hint, panelX + panelW/2 - hintW/2, panelY+panelH-24, hintCol);

    SDL_RenderPresent(r);
}
