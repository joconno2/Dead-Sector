#include "ShipSelectScene.hpp"
#include "MapScene.hpp"
#include "MainMenuScene.hpp"
#include "CombatScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/Constants.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "math/Vec2.hpp"
#include "steam/SteamManager.hpp"

#include <SDL.h>
#include <cmath>
#include <string>

// ---------------------------------------------------------------------------
// Unlock logic
// ---------------------------------------------------------------------------

bool ShipSelectScene::isUnlocked(int idx, const SaveData& save) const {
    HullStats s = statsForHull(HULL_ORDER[idx]);
    if (s.id == std::string("DELTA")) return true;  // starter hull always available
    return save.hasPurchase(std::string("HULL_") + s.id);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ShipSelectScene::onEnter(SceneContext& ctx) {
    m_time   = 0.f;
    m_pulse  = 0.f;
    // Pre-select cursor to match saved active hull
    m_cursor = 0;
    if (ctx.saveData) {
        for (int i = 0; i < NUM_HULLS; ++i) {
            HullStats s = statsForHull(HULL_ORDER[i]);
            if (ctx.saveData->activeHull == s.id) { m_cursor = i; break; }
        }
        // ACH_ALL_HULLS: check if every hull is now unlocked
        if (ctx.steam) {
            bool allUnlocked = true;
            for (int i = 0; i < NUM_HULLS; ++i)
                if (!isUnlocked(i, *ctx.saveData)) { allUnlocked = false; break; }
            if (allUnlocked) {
                ctx.steam->unlockAchievement(ACH_ALL_HULLS);
                ctx.steam->checkCompletionist();
            }
        }
    }
}

void ShipSelectScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    bool left = false, right = false, select = false, back = false;

    if (ev.type == SDL_KEYDOWN) {
        switch (ev.key.keysym.sym) {
        case SDLK_LEFT:  case SDLK_a: left   = true; break;
        case SDLK_RIGHT: case SDLK_d: right  = true; break;
        case SDLK_RETURN: case SDLK_SPACE: select = true; break;
        case SDLK_ESCAPE: back = true; break;
        default: break;
        }
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (ev.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  left   = true; break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: right  = true; break;
        case SDL_CONTROLLER_BUTTON_A:          select = true; break;
        case SDL_CONTROLLER_BUTTON_B:
        case SDL_CONTROLLER_BUTTON_START:      back   = true; break;
        default: break;
        }
    }

    if (left)  { m_cursor = (m_cursor - 1 + NUM_HULLS) % NUM_HULLS; }
    if (right) { m_cursor = (m_cursor + 1) % NUM_HULLS; }
    if (select) confirm(ctx);
    if (back)  {
        // Return to main menu
        ctx.scenes->replace(std::make_unique<MainMenuScene>());
    }
}

void ShipSelectScene::update(float dt, SceneContext&) {
    m_time  += dt;
    m_pulse += dt * 2.5f;
}

void ShipSelectScene::confirm(SceneContext& ctx) {
    if (!ctx.saveData) return;
    HullStats s = statsForHull(HULL_ORDER[m_cursor]);
    if (!isUnlocked(m_cursor, *ctx.saveData)) return; // locked — ignore

    ctx.saveData->activeHull = s.id;
    SaveSystem::save(*ctx.saveData);

    if (m_endless) {
        NodeConfig cfg;
        cfg.nodeId        = -1;
        cfg.tier          = 1;
        cfg.objective     = NodeObjective::Sweep;
        cfg.sweepTarget   = 999999;
        cfg.traceTickRate = 0.17f;
        cfg.endless       = true;
        ctx.scenes->replace(std::make_unique<CombatScene>(cfg));
    } else {
        ctx.scenes->replace(std::make_unique<MapScene>(*ctx.nodeMap));
    }
}

// ---------------------------------------------------------------------------
// Rendering helpers
// ---------------------------------------------------------------------------

void ShipSelectScene::renderShipSilhouette(SDL_Renderer* r,
    const HullStats& stats, HullType hull,
    float cx, float cy, float scale,
    bool golden, bool locked, float time) const
{
    auto verts = hullVerts(hull);
    if (verts.empty()) return;

    // Transform to screen space
    std::vector<SDL_Point> pts;
    for (auto& v : verts) {
        pts.push_back({ (int)(cx + v.x * scale), (int)(cy + v.y * scale) });
    }
    pts.push_back(pts.front()); // close

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    if (locked) {
        // Dim red, low alpha
        SDL_SetRenderDrawColor(r, 80, 20, 20, 120);
        SDL_RenderDrawLines(r, pts.data(), (int)pts.size());
    } else if (golden) {
        // Gold glow — draw layered lines with decreasing alpha
        float pulse = 0.6f + 0.4f * std::sin(time * 3.f);
        for (int layer = 4; layer >= 1; --layer) {
            Uint8 a = (Uint8)(60 * pulse / layer);
            SDL_SetRenderDrawColor(r, 255, (Uint8)(180 + 30 * pulse), 30, a);
            // Inflate each vert by layer pixels outward from center
            std::vector<SDL_Point> glow;
            for (auto& v : verts) {
                float dx = v.x * scale;
                float dy = v.y * scale;
                float len = std::sqrt(dx*dx + dy*dy);
                if (len > 0) { float f = (len + layer) / len; dx *= f; dy *= f; }
                glow.push_back({ (int)(cx + dx), (int)(cy + dy) });
            }
            glow.push_back(glow.front());
            SDL_RenderDrawLines(r, glow.data(), (int)glow.size());
        }
        // Core bright gold line
        SDL_SetRenderDrawColor(r, 255, 210, 60, 255);
        SDL_RenderDrawLines(r, pts.data(), (int)pts.size());
    } else {
        // Normal — cyan-white glow
        float pulse = 0.5f + 0.5f * std::sin(time * 2.f);
        SDL_SetRenderDrawColor(r, 80, (Uint8)(200 + 40 * pulse), 255, 180);
        SDL_RenderDrawLines(r, pts.data(), (int)pts.size());
        SDL_SetRenderDrawColor(r, 180, 240, 255, (Uint8)(80 * pulse));
        SDL_RenderDrawLines(r, pts.data(), (int)pts.size());
    }
}

void ShipSelectScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;
    if (!vr || !r) return;

    vr->clear();
    vr->drawGrid(0, 40, 20);
    vr->drawCRTOverlay();

    if (!ctx.hud) { SDL_RenderPresent(r); return; }

    // Title
    SDL_Color titleCol = { 0, 220, 150, 255 };
    ctx.hud->drawLabel("// SELECT HULL //", Constants::SCREEN_W / 2 - 120, 28, titleCol);

    // Draw 5 ships spread across the screen
    constexpr float SLOT_W = Constants::SCREEN_WF / NUM_HULLS;
    constexpr float SHIP_Y = Constants::SCREEN_HF * 0.38f;
    constexpr float SCALE  = 1.4f;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < NUM_HULLS; ++i) {
        HullType  ht      = HULL_ORDER[i];
        HullStats stats   = statsForHull(ht);
        float     cx      = SLOT_W * i + SLOT_W * 0.5f;
        bool      locked  = ctx.saveData && !isUnlocked(i, *ctx.saveData);
        bool      golden  = ctx.saveData && ctx.saveData->isGolden(stats.id);
        bool      sel     = (i == m_cursor);

        // Selection highlight box
        if (sel) {
            float pulse = 0.3f + 0.3f * std::sin(m_pulse);
            SDL_SetRenderDrawColor(r, 0, (Uint8)(180 + 60 * pulse), (Uint8)(120 + 80 * pulse),
                                   (Uint8)(40 + 30 * pulse));
            SDL_Rect box = { (int)(cx - SLOT_W*0.45f), (int)(SHIP_Y - 52),
                             (int)(SLOT_W * 0.9f), 104 };
            SDL_RenderFillRect(r, &box);
            SDL_SetRenderDrawColor(r, 0, (Uint8)(200 + 55 * pulse), 180, 200);
            SDL_RenderDrawRect(r, &box);
        }

        renderShipSilhouette(r, stats, ht, cx, SHIP_Y, SCALE * (sel ? 1.2f : 1.f),
                              golden, locked, m_time);

        // Hull name below ship
        SDL_Color nameCol = locked ? SDL_Color{80,30,30,180}
                          : golden ? SDL_Color{255,200,50,255}
                          :          SDL_Color{0,200,140,220};
        int nameX = (int)(cx - 50);
        ctx.hud->drawLabel(stats.name, nameX, (int)(SHIP_Y + 60), nameCol);

        // Lock indicator
        if (locked) {
            SDL_Color lockCol = { 120, 40, 40, 200 };
            ctx.hud->drawLabel("[LOCKED]", nameX + 4, (int)(SHIP_Y + 80), lockCol);
        }
    }

    // Selected hull stats panel
    HullStats sel = statsForHull(HULL_ORDER[m_cursor]);
    bool locked   = ctx.saveData && !isUnlocked(m_cursor, *ctx.saveData);
    bool golden   = ctx.saveData && ctx.saveData->isGolden(sel.id);

    int panelW = 560, panelH = 210;
    int panelX = Constants::SCREEN_W / 2 - panelW / 2;
    int panelY = (int)(Constants::SCREEN_HF * 0.62f);

    SDL_SetRenderDrawColor(r, 0, 18, 12, 210);
    SDL_Rect panel = { panelX, panelY, panelW, panelH };
    SDL_RenderFillRect(r, &panel);
    SDL_Color borderCol = golden ? SDL_Color{200,160,30,220}
                        : locked ? SDL_Color{80,30,30,180}
                        :          SDL_Color{0,180,120,200};
    SDL_SetRenderDrawColor(r, borderCol.r, borderCol.g, borderCol.b, borderCol.a);
    SDL_RenderDrawRect(r, &panel);

    int tx = panelX + 16, ty = panelY + 12;
    SDL_Color hdrCol  = golden ? SDL_Color{255,200,50,255} : SDL_Color{0,220,140,255};
    SDL_Color statCol = { 100, 180, 140, 220 };
    SDL_Color valCol  = { 220, 220, 80, 255 };
    SDL_Color dimCol  = { 80, 120, 100, 180 };

    std::string hdrStr = std::string(sel.id) + "  //  " + sel.name;
    if (golden) hdrStr += "  [GOLDEN]";
    ctx.hud->drawLabel(hdrStr.c_str(), tx, ty, hdrCol); ty += 24;

    // Stat bars (simple text representation)
    auto pct = [](float v) -> std::string {
        int p = (int)((v - 1.f) * 100.f + 0.5f);
        return (p >= 0 ? "+" : "") + std::to_string(p) + "%";
    };

    if (locked) {
        // Show unlock requirement
        SDL_Color reqCol  = { 180, 60, 60, 220 };
        SDL_Color shopCol = { 80, 160, 220, 220 };
        ctx.hud->drawLabel("[ LOCKED ]", tx, ty, reqCol); ty += 22;
        ctx.hud->drawLabel("Purchase this hull in the SHOP to unlock.", tx, ty, shopCol); ty += 20;
    } else {
        ctx.hud->drawLabel(("SPEED:      " + pct(sel.speedMult)).c_str(),    tx,       ty, sel.speedMult    >= 1.f ? valCol : statCol);
        ctx.hud->drawLabel(("THRUST:     " + pct(sel.thrustMult)).c_str(),   tx + 210, ty, sel.thrustMult   >= 1.f ? valCol : statCol);
        ty += 22;
        ctx.hud->drawLabel(("ROTATION:   " + pct(sel.rotMult)).c_str(),      tx,       ty, sel.rotMult      >= 1.f ? valCol : statCol);
        ctx.hud->drawLabel(("SHOT SPEED: " + pct(sel.projSpeedMult)).c_str(),tx + 210, ty, sel.projSpeedMult>= 1.f ? valCol : statCol);
        ty += 22;
        ctx.hud->drawLabel(("HITBOX:     " + pct(sel.radiusMult)).c_str(),   tx,       ty, sel.radiusMult   <= 1.f ? valCol : statCol);
        std::string ammoStr = "AMMO:       " + std::to_string(sel.startingAmmo);
        ctx.hud->drawLabel(ammoStr.c_str(), tx + 210, ty, statCol);
        ty += 22;
        if (sel.extraLives > 0) {
            ctx.hud->drawLabel(("BONUS LIFE: +" + std::to_string(sel.extraLives)).c_str(), tx, ty, valCol);
        }
    }

    // Prompt
    float blink = 0.5f + 0.5f * std::sin(m_time * 4.f);
    SDL_Color promptCol = { 0, (Uint8)(140 + 60 * blink), (Uint8)(120 + 60 * blink), 220 };
    const char* prompt = locked ? ">> [LOCKED] USE ARROW KEYS TO BROWSE <<"
                                : ">> ENTER/A TO DEPLOY  |  L/R TO BROWSE <<";
    ctx.hud->drawLabel(prompt, panelX + 10, panelY + panelH - 22, promptCol);

    SDL_RenderPresent(r);
}
