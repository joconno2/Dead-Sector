#include "HUD.hpp"
#include "systems/ProgramSystem.hpp"
#include "systems/ModSystem.hpp"
#include "core/Constants.hpp"
#include "core/Mods.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <vector>

HUD::HUD(SDL_Renderer* renderer, const char* fontPath, int fontSize)
    : m_renderer(renderer)
{
    m_font = TTF_OpenFont(fontPath, fontSize);
    if (!m_font)
        std::cerr << "HUD: could not load font '" << fontPath << "': " << TTF_GetError() << "\n";
}

HUD::~HUD() {
    if (m_font) TTF_CloseFont(m_font);
}

void HUD::drawText(const std::string& text, int x, int y, SDL_Color color) {
    if (!m_font) return;
    SDL_Surface* surf = TTF_RenderText_Blended(m_font, text.c_str(), color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surf);
    SDL_FreeSurface(surf);
    if (!tex) return;
    int w = 0, h = 0;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(m_renderer, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

// Public alias — for use by other scenes
void HUD::drawLabel(const std::string& text, int x, int y, SDL_Color color) {
    drawText(text, x, y, color);
}

void HUD::render(int score, float tracePercent,
                 const ProgramSystem* programs, const ModSystem* mods) {
    SDL_Color green = { Constants::COL_HUD_R, Constants::COL_HUD_G, Constants::COL_HUD_B, 255 };

    // Score — top left
    std::ostringstream ss;
    ss << "SCORE: " << std::setw(6) << std::setfill('0') << score;
    drawText(ss.str(), 16, 12, green);

    // Active mods — top right (small compact list)
    if (mods && !mods->all().empty()) {
        const auto& allMods = mods->all();
        // Deduplicate
        std::vector<std::pair<ModID,int>> unique;
        for (ModID m : allMods) {
            bool found = false;
            for (auto& p : unique) if (p.first == m) { p.second++; found = true; break; }
            if (!found) unique.push_back({m, 1});
        }
        for (int i = 0; i < (int)unique.size() && i < 5; ++i) {
            const ModDef& def = getModDef(unique[i].first);
            uint8_t mr, mg, mb;
            switch (def.category) {
                case ModCategory::Chassis: mr=40;  mg=200; mb=120; break;
                case ModCategory::Weapon:  mr=220; mg=180; mb=30;  break;
                case ModCategory::Neural:  mr=180; mg=60;  mb=255; break;
                default:                   mr=140; mg=140; mb=140; break;
            }
            std::string label = def.name;
            if (unique[i].second > 1) label += " x" + std::to_string(unique[i].second);
            drawText(label, Constants::SCREEN_W - 180, 12 + i * 22, {mr,mg,mb,200});
        }
    }

    // Trace bar — bottom left
    drawTraceBar(tracePercent);

    // Program slots — bottom right
    if (programs) drawProgramSlots(programs);
}

void HUD::drawTraceBar(float tracePct) {
    constexpr int   BAR_X  = 16;
    constexpr int   BAR_Y  = Constants::SCREEN_H - 38;
    constexpr int   BAR_W  = 220;
    constexpr int   BAR_H  = 12;
    constexpr float SEGS[] = { 0.25f, 0.50f, 0.75f };

    SDL_Color labelCol = {200, 60, 60, 255};
    drawText("TRACE", BAR_X, BAR_Y - 22, labelCol);

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 40, 10, 10, 180);
    SDL_Rect bg = {BAR_X, BAR_Y, BAR_W, BAR_H};
    SDL_RenderFillRect(m_renderer, &bg);

    float t     = std::min(tracePct / 100.f, 1.f);
    int   fillW = static_cast<int>(BAR_W * t);

    if (fillW > 0) {
        uint8_t r, g, b;
        if      (t < 0.25f) { r = 60;  g = 220; b = 80;  }
        else if (t < 0.50f) { r = 220; g = 210; b = 30;  }
        else if (t < 0.75f) { r = 255; g = 120; b = 20;  }
        else                { r = 255; g = 30;  b = 30;  }

        SDL_SetRenderDrawColor(m_renderer, r, g, b, 220);
        SDL_Rect fill = {BAR_X, BAR_Y, fillW, BAR_H};
        SDL_RenderFillRect(m_renderer, &fill);
    }

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 200);
    for (float seg : SEGS) {
        int tickX = BAR_X + static_cast<int>(BAR_W * seg);
        SDL_RenderDrawLine(m_renderer, tickX, BAR_Y, tickX, BAR_Y + BAR_H);
    }

    SDL_SetRenderDrawColor(m_renderer, 120, 40, 40, 200);
    SDL_RenderDrawRect(m_renderer, &bg);
}

void HUD::drawProgramSlots(const ProgramSystem* prog) {
    // Three slots, bottom-right of screen
    constexpr int SLOT_W    = 90;
    constexpr int SLOT_H    = 36;
    constexpr int SLOT_PAD  = 6;
    constexpr int SLOTS     = ProgramSystem::SLOTS;
    constexpr int TOTAL_W   = SLOTS * SLOT_W + (SLOTS - 1) * SLOT_PAD;
    constexpr int BASE_X    = Constants::SCREEN_W - TOTAL_W - 16;
    constexpr int BASE_Y    = Constants::SCREEN_H - SLOT_H - 14;

    const char* KEYS[3] = { "[Q]", "[E]", "[R]" };

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < SLOTS; ++i) {
        int sx = BASE_X + i * (SLOT_W + SLOT_PAD);
        int sy = BASE_Y;

        bool empty = prog->isEmpty(i);
        bool onCD  = !empty && prog->onCooldown(i);
        float cdR  = empty ? 0.f : prog->cooldownRatio(i);

        // Slot background — dimmer when empty
        uint8_t bgA = empty ? 60 : (onCD ? 120 : 180);
        SDL_SetRenderDrawColor(m_renderer, 5, empty ? 10 : 20, empty ? 15 : 30, bgA);
        SDL_Rect slot = { sx, sy, SLOT_W, SLOT_H };
        SDL_RenderFillRect(m_renderer, &slot);

        // Cooldown fill overlay
        if (onCD && cdR > 0.f) {
            int fillH = (int)(SLOT_H * cdR);
            SDL_SetRenderDrawColor(m_renderer, 80, 10, 10, 140);
            SDL_Rect cd = { sx, sy + SLOT_H - fillH, SLOT_W, fillH };
            SDL_RenderFillRect(m_renderer, &cd);
        }

        // Border
        if (empty)
            SDL_SetRenderDrawColor(m_renderer, 30, 40, 35, 120);
        else if (onCD)
            SDL_SetRenderDrawColor(m_renderer, 60, 40, 40, 180);
        else
            SDL_SetRenderDrawColor(m_renderer, 20, 200, 170, 220);
        SDL_RenderDrawRect(m_renderer, &slot);

        SDL_Color keyCol  = empty ? SDL_Color{50,70,60,150} : SDL_Color{100,200,180,255};
        drawText(KEYS[i], sx + 4, sy + 2, keyCol);

        if (empty) {
            drawText("------", sx + 4, sy + 18, {40, 60, 50, 150});
        } else {
            const auto& def = getProgramDef(prog->slotID(i));
            SDL_Color nameCol = onCD ? SDL_Color{80,80,80,200} : SDL_Color{220,220,200,255};
            drawText(def.name, sx + 4, sy + 18, nameCol);
        }
    }
}
