#include "DebugOverlay.hpp"
#include "core/SaveSystem.hpp"
#include "core/Constants.hpp"
#include <SDL_ttf.h>
#include <string>
#include <cstring>

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static void drawRect(SDL_Renderer* r, int x, int y, int w, int h,
                     Uint8 R, Uint8 G, Uint8 B, Uint8 A)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, R, G, B, A);
    SDL_Rect rect{ x, y, w, h };
    SDL_RenderFillRect(r, &rect);
}

static void drawBorder(SDL_Renderer* r, int x, int y, int w, int h,
                       Uint8 R, Uint8 G, Uint8 B)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    SDL_Rect rect{ x, y, w, h };
    SDL_RenderDrawRect(r, &rect);
}

static void drawText(SDL_Renderer* renderer, TTF_Font* font,
                     const std::string& text, int x, int y, SDL_Color col)
{
    if (!font) return;
    SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), col);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
        SDL_Rect dst{ x, y, surf->w, surf->h };
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

// ---------------------------------------------------------------------------
// handleEvent
// ---------------------------------------------------------------------------

void DebugOverlay::handleEvent(SDL_Event& ev, SceneContext& ctx)
{
    // Keyboard toggle: F12
    if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_F12) {
        m_open = !m_open;
        m_selected = 0;
        return;
    }

    if (!m_open) return;

    if (ev.type == SDL_KEYDOWN) {
        switch (ev.key.keysym.scancode) {
            case SDL_SCANCODE_UP:
                m_selected = (m_selected - 1 + NUM_ITEMS) % NUM_ITEMS; break;
            case SDL_SCANCODE_DOWN:
                m_selected = (m_selected + 1) % NUM_ITEMS; break;
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_SPACE:
                executeItem(m_selected, ctx);
                break;
            case SDL_SCANCODE_ESCAPE:
                m_open = false; break;
            default: break;
        }
    }

    if (ev.type == SDL_CONTROLLERBUTTONDOWN && ctx.controller) {
        switch (ev.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                m_selected = (m_selected - 1 + NUM_ITEMS) % NUM_ITEMS; break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                m_selected = (m_selected + 1) % NUM_ITEMS; break;
            case SDL_CONTROLLER_BUTTON_A:
                executeItem(m_selected, ctx); break;
            case SDL_CONTROLLER_BUTTON_B:
                m_open = false; break;
            default: break;
        }
    }
}

// ---------------------------------------------------------------------------
// update — poll LB+RB held simultaneously to open
// ---------------------------------------------------------------------------

void DebugOverlay::update(float dt, SceneContext& ctx)
{
    if (!ctx.controller) return;

    bool lb = SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)  != 0;
    bool rb = SDL_GameControllerGetButton(ctx.controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) != 0;

    if (lb && rb) {
        m_lbTimer += dt;
        if (m_lbTimer >= 0.5f && !m_open) {
            m_open     = true;
            m_selected = 0;
            m_lbTimer  = 0.f;
        }
    } else {
        m_lbTimer = 0.f;
    }
    (void)m_rbTimer; // unused, kept for readability
}

// ---------------------------------------------------------------------------
// render
// ---------------------------------------------------------------------------

void DebugOverlay::render(SDL_Renderer* renderer, TTF_Font* font) const
{
    if (!m_open) return;

    const int W = 420;
    const int H = 46 + NUM_ITEMS * 40 + 20;
    const int X = (Constants::SCREEN_W - W) / 2;
    const int Y = (Constants::SCREEN_H - H) / 2;

    // Dark translucent background
    drawRect(renderer, X, Y, W, H, 0, 0, 0, 210);
    drawBorder(renderer, X, Y, W, H, 255, 50, 50);
    // inner glow border
    drawBorder(renderer, X+2, Y+2, W-4, H-4, 120, 20, 20);

    // Title
    SDL_Color titleCol{ 255, 60, 60, 255 };
    const char* title = "// DEBUG CONSOLE //";
    int titleX = X + W/2 - (int)(strlen(title) * 7);
    drawText(renderer, font, title, titleX, Y + 10, titleCol);

    // Items
    for (int i = 0; i < NUM_ITEMS; ++i) {
        int iy = Y + 44 + i * 40;
        bool sel = (i == m_selected);

        if (sel) {
            drawRect(renderer, X + 8, iy - 2, W - 16, 30, 180, 30, 30, 160);
            drawBorder(renderer, X + 8, iy - 2, W - 16, 30, 255, 80, 80);
        }

        SDL_Color col = sel ? SDL_Color{255, 220, 220, 255} : SDL_Color{160, 80, 80, 255};
        std::string label = std::string(sel ? "> " : "  ") + LABELS[i];
        drawText(renderer, font, label, X + 18, iy, col);
    }

    // Footer hint
    SDL_Color hint{ 80, 40, 40, 255 };
    drawText(renderer, font, "[A/ENTER] confirm  [B/ESC] close", X + 16, Y + H - 22, hint);
}

// ---------------------------------------------------------------------------
// executeItem
// ---------------------------------------------------------------------------

void DebugOverlay::executeItem(int idx, SceneContext& ctx)
{
    switch (idx) {
        case 0:
            ctx.debugInvincible = !ctx.debugInvincible;
            break;
        case 1:
            if (ctx.saveData) {
                ctx.saveData->credits += 10000;
                SaveSystem::save(*ctx.saveData);
            }
            break;
        case 2:
            if (ctx.saveData) {
                *ctx.saveData = SaveData{};
                SaveSystem::save(*ctx.saveData);
                ctx.debugInvincible = false;
            }
            break;
    }
}
