#pragma once
#include <SDL.h>
#include <cmath>
#include <string>
#include "core/Programs.hpp"
#include "core/Mods.hpp"
#include "systems/ModSystem.hpp"
#include "systems/ProgramSystem.hpp"
#include "core/SaveSystem.hpp"
#include "entities/Avatar.hpp"
#include "renderer/HUD.hpp"

// Draws the current run build (hull stats, programs, mods) into the rect
// (px, py, pw, ph).  pulse is a 0..1 sine value for animation.
inline void renderBuildChart(SDL_Renderer* r, HUD* hud,
    ModSystem* mods, ProgramSystem* programs, SaveData* save,
    float pulse, int px, int py, int pw, int ph)
{
    if (!r || !hud) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Panel background + border
    SDL_SetRenderDrawColor(r, 2, 14, 10, 215);
    SDL_Rect bg = { px, py, pw, ph };
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawColor(r, 0, (Uint8)(100 + 60*pulse), 70, 200);
    SDL_RenderDrawRect(r, &bg);

    int ry = py + 8;
    int lx = px + 10;
    int rx = px + pw - 10;

    auto divider = [&]() {
        SDL_SetRenderDrawColor(r, 0, 50, 35, 110);
        SDL_RenderDrawLine(r, lx, ry, rx, ry);
        ry += 6;
    };

    // Title
    SDL_Color titleC = { 0, (Uint8)(190 + 65*pulse), 140, 255 };
    hud->drawLabel("// BUILD //", lx, ry, titleC);
    ry += 20;
    divider();

    // ── Hull ──────────────────────────────────────────────────
    SDL_Color secC  = { 50, 110, 85, 160 };
    SDL_Color valC  = { 200, 200, 75, 220 };
    SDL_Color dimC  = { 90, 130, 110, 200 };
    hud->drawLabel("HULL", lx, ry, secC);

    if (save) {
        HullType  ht = hullFromString(save->activeHull);
        HullStats hs = statsForHull(ht);
        SDL_Color hullC = { 80, 210, 150, 230 };
        hud->drawLabel(hs.name, lx + 46, ry, hullC);
        ry += 20;

        // Effective multipliers (hull × mod)
        float tm = hs.thrustMult    * (mods ? mods->thrustMult()    : 1.f);
        float sm = hs.speedMult     * (mods ? mods->maxSpeedMult()  : 1.f);
        float rm = hs.rotMult       * (mods ? mods->rotMult()       : 1.f);
        float pm = hs.projSpeedMult * (mods ? mods->projSpeedMult() : 1.f);
        float cd = mods ? mods->cdMult() : 1.f; // <1 = shorter cooldowns

        auto pct = [](float v) -> std::string {
            int p = (int)((v - 1.f) * 100.f + 0.5f);
            return (p >= 0 ? "+" : "") + std::to_string(p) + "%";
        };
        auto cdrStr = [&]() -> std::string {
            int p = (int)((1.f - cd) * 100.f + 0.5f);
            return (p > 0 ? "-" : "+") + std::to_string(std::abs(p)) + "%";
        };

        std::string ln1 = "THR " + pct(tm) + "  SPD " + pct(sm);
        std::string ln2 = "ROT " + pct(rm) + "  SHT " + pct(pm);
        std::string ln3 = "CDR " + cdrStr();
        if (hs.extraLives > 0)
            ln3 += "  +" + std::to_string(hs.extraLives) + " LIFE";

        hud->drawLabel(ln1.c_str(), lx, ry, dimC); ry += 18;
        hud->drawLabel(ln2.c_str(), lx, ry, dimC); ry += 18;
        hud->drawLabel(ln3.c_str(), lx, ry, dimC); ry += 20;
    } else {
        ry += 56;
    }
    divider();

    // ── Programs ──────────────────────────────────────────────
    hud->drawLabel("PROGRAMS", lx, ry, secC); ry += 18;
    if (programs) {
        for (int i = 0; i < ProgramSystem::MAX_SLOTS; ++i) {
            if (ry > py + ph - 28) break;
            if (programs->isEmpty(i)) {
                hud->drawLabel("[EMPTY]", lx + 14, ry, { 55, 75, 65, 130 });
            } else {
                const ProgramDef& pd = getProgramDef(programs->slotID(i));
                uint8_t R, G, B;
                abilityTypeColor(pd.abilityType, R, G, B);
                SDL_SetRenderDrawColor(r, R, G, B, 200);
                SDL_Rect dot = { lx, ry + 4, 8, 9 };
                SDL_RenderFillRect(r, &dot);
                SDL_Color pc = { R, G, B, 245 };
                std::string entry = std::string(pd.name);
                hud->drawLabel(entry.c_str(), lx + 14, ry, pc);

                // Rarity dots (right-aligned in remaining space)
                uint8_t rr, rg, rb;
                rarityColor(static_cast<ModRarity>(pd.rarity), rr, rg, rb);
                int dots = (int)pd.rarity + 1;  // Common=1, Uncommon=2, Rare=3, Legendary=4
                int dotX = rx - dots * 9;
                for (int d = 0; d < dots; ++d) {
                    SDL_SetRenderDrawColor(r, rr, rg, rb, 220);
                    SDL_Rect dr = { dotX + d*9, ry + 6, 6, 6 };
                    SDL_RenderFillRect(r, &dr);
                }
            }
            ry += 18;
        }
    }
    ry += 4;
    divider();

    // ── Mods ──────────────────────────────────────────────────
    hud->drawLabel("MODS", lx, ry, secC); ry += 18;
    if (mods && !mods->all().empty()) {
        for (ModID mid : mods->all()) {
            if (ry > py + ph - 20) {
                hud->drawLabel("...", lx, ry, { 80, 80, 80, 180 });
                break;
            }
            const ModDef& md = getModDef(mid);
            uint8_t R, G, B;
            abilityTypeColor(md.abilityType, R, G, B);
            SDL_SetRenderDrawColor(r, R, G, B, 180);
            SDL_Rect dot = { lx, ry + 4, 8, 9 };
            SDL_RenderFillRect(r, &dot);
            SDL_Color mc = { R, G, B, 235 };
            hud->drawLabel(md.name, lx + 14, ry, mc);

            // Rarity dots
            uint8_t rr, rg, rb;
            rarityColor(md.rarity, rr, rg, rb);
            int dots = (int)md.rarity + 1;
            int dotX = rx - dots * 9;
            for (int d = 0; d < dots; ++d) {
                SDL_SetRenderDrawColor(r, rr, rg, rb, 220);
                SDL_Rect dr = { dotX + d*9, ry + 6, 6, 6 };
                SDL_RenderFillRect(r, &dr);
            }
            ry += 18;
        }
    } else {
        hud->drawLabel("NONE", lx, ry, { 55, 75, 65, 140 });
    }
}
