#include "LoadoutScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "systems/ModSystem.hpp"
#include "core/Constants.hpp"
#include "renderer/HUD.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/BuildChart.hpp"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <random>

// ---------------------------------------------------------------------------
// Layout constants
// ---------------------------------------------------------------------------
static constexpr int COLS     = 3;
static constexpr int ROWS     = 2;
static constexpr int CARD_W   = 200;
static constexpr int CARD_H   = 116;
static constexpr int CARD_PAD = 18;

// Build chart on the right side
static constexpr int CHART_W  = 272;
static constexpr int CHART_GAP = 22;

// Center the (grid + gap + chart) block horizontally
static constexpr int GRID_W = COLS * CARD_W + (COLS - 1) * CARD_PAD;   // 636
static constexpr int BLOCK_W = GRID_W + CHART_GAP + CHART_W;            // 930
static constexpr int GRID_X  = (Constants::SCREEN_W - BLOCK_W) / 2;    // 175
static constexpr int GRID_Y  = (Constants::SCREEN_H - (ROWS * CARD_H + (ROWS - 1) * CARD_PAD)) / 2 + 16;
static constexpr int CHART_X = GRID_X + GRID_W + CHART_GAP;
static constexpr int CHART_Y = GRID_Y - 4;
static constexpr int CHART_H = ROWS * CARD_H + (ROWS - 1) * CARD_PAD + 8;

// Upgrade-phase card layout
static constexpr int UPG_CARD_W   = 260;
static constexpr int UPG_CARD_H   = 160;
static constexpr int UPG_CARD_PAD = 20;
static constexpr int UPG_TOTAL    = LoadoutScene::MOD_OFFERS * UPG_CARD_W
                                  + (LoadoutScene::MOD_OFFERS - 1) * UPG_CARD_PAD;
static constexpr int UPG_X0 = (Constants::SCREEN_W - UPG_TOTAL) / 2;
static constexpr int UPG_Y0 = (Constants::SCREEN_H - UPG_CARD_H) / 2 + 30;

// ---------------------------------------------------------------------------

LoadoutScene::LoadoutScene(NodeConfig cfg) : m_cfg(cfg) {}

void LoadoutScene::onEnter(SceneContext&) {
    m_picks.clear();
    m_cursor    = 0;
    m_pulse     = 0.f;
    m_phase     = Phase::Programs;
    m_modCursor = 0;
    m_modOffered.clear();
    buildPool();
}

void LoadoutScene::onExit() {}

// ---------------------------------------------------------------------------
// Pool generation — 6 unique programs drawn at random
// ---------------------------------------------------------------------------
void LoadoutScene::buildPool() {
    std::vector<ProgramID> available;
    for (int i = 0; i < (int)ProgramID::COUNT; ++i)
        available.push_back(static_cast<ProgramID>(i));
    std::mt19937 rng(std::random_device{}());
    std::shuffle(available.begin(), available.end(), rng);
    for (int i = 0; i < POOL_SIZE; ++i)
        m_pool[i] = available[i];
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
void LoadoutScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    // ---- Starting upgrade pick phase ----
    if (m_phase == Phase::StartingUpgrade) {
        auto dir = [&](int d) {
            m_modCursor = (m_modCursor + d + MOD_OFFERS) % MOD_OFFERS;
        };
        if (ev.type == SDL_KEYDOWN) {
            const auto sc = ev.key.keysym.scancode;
            if (sc == SDL_SCANCODE_LEFT  || sc == SDL_SCANCODE_A) dir(-1);
            if (sc == SDL_SCANCODE_RIGHT || sc == SDL_SCANCODE_D) dir(+1);
            if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_SPACE) pickStartingMod(ctx);
        } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
            if (ev.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)  dir(-1);
            if (ev.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) dir(+1);
            if (ev.cbutton.button == SDL_CONTROLLER_BUTTON_A
             || ev.cbutton.button == SDL_CONTROLLER_BUTTON_START) pickStartingMod(ctx);
        }
        return;
    }

    // ---- Program selection phase ----
    int dCol = 0, dRow = 0;

    auto handleButton = [&](int btn) {
        if      (btn == SDL_CONTROLLER_BUTTON_DPAD_LEFT)  dCol = -1;
        else if (btn == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) dCol =  1;
        else if (btn == SDL_CONTROLLER_BUTTON_DPAD_UP)    dRow = -1;
        else if (btn == SDL_CONTROLLER_BUTTON_DPAD_DOWN)  dRow =  1;
        else if (btn == SDL_CONTROLLER_BUTTON_A)          toggleSelect(ctx);
        else if (btn == SDL_CONTROLLER_BUTTON_START)      confirm(ctx);
    };

    if (ev.type == SDL_KEYDOWN) {
        const auto sc = ev.key.keysym.scancode;
        if      (sc == SDL_SCANCODE_LEFT  || sc == SDL_SCANCODE_A) dCol = -1;
        else if (sc == SDL_SCANCODE_RIGHT || sc == SDL_SCANCODE_D) dCol =  1;
        else if (sc == SDL_SCANCODE_UP    || sc == SDL_SCANCODE_W) dRow = -1;
        else if (sc == SDL_SCANCODE_DOWN  || sc == SDL_SCANCODE_S) dRow =  1;
        else if (sc == SDL_SCANCODE_SPACE || sc == SDL_SCANCODE_RETURN) toggleSelect(ctx);
        else if (sc == SDL_SCANCODE_TAB)  confirm(ctx);
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        handleButton(ev.cbutton.button);
    }

    if (dCol != 0 || dRow != 0) moveCursor(dCol, dRow);
}

void LoadoutScene::moveCursor(int dCol, int dRow) {
    int col = m_cursor % COLS;
    int row = m_cursor / COLS;
    col = (col + dCol + COLS) % COLS;
    row = (row + dRow + ROWS) % ROWS;
    m_cursor = row * COLS + col;
}

void LoadoutScene::toggleSelect(SceneContext& ctx) {
    auto it = std::find(m_picks.begin(), m_picks.end(), m_cursor);
    if (it != m_picks.end()) {
        m_picks.erase(it);
        return;
    }
    if ((int)m_picks.size() >= PICKS) return;
    m_picks.push_back(m_cursor);
    if ((int)m_picks.size() == PICKS) confirm(ctx);
}

void LoadoutScene::confirm(SceneContext& ctx) {
    if ((int)m_picks.size() < PICKS) return;

    if (ctx.mods && ctx.mods->all().empty()) {
        m_modOffered = ctx.mods->buildOfferPool(MOD_OFFERS, !ctx.mods->passiveFull());
        m_modCursor  = 0;
        m_phase      = Phase::StartingUpgrade;
    } else {
        launchCombat(ctx);
    }
}

void LoadoutScene::pickStartingMod(SceneContext& ctx) {
    if (m_modCursor >= 0 && m_modCursor < (int)m_modOffered.size()) {
        if (ctx.mods) ctx.mods->add(m_modOffered[m_modCursor]);
    }
    launchCombat(ctx);
}

void LoadoutScene::launchCombat(SceneContext& ctx) {
    if (ctx.programs) {
        for (int i = 0; i < PICKS && i < (int)m_picks.size(); ++i)
            ctx.programs->add(m_pool[m_picks[i]]);
    }
    ctx.scenes->replace(std::make_unique<CombatScene>(m_cfg));
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------
void LoadoutScene::update(float dt, SceneContext&) {
    m_pulse += dt * 3.f;
}

// ---------------------------------------------------------------------------
// Render helpers — ability-type color stripe card
// ---------------------------------------------------------------------------

// Draws 1–4 rarity dots right-aligned inside [dotX .. dotX+maxW]
static void drawRarityDots(SDL_Renderer* r, ModRarity rar, int rightX, int y) {
    uint8_t rr, rg, rb;
    rarityColor(rar, rr, rg, rb);
    int dots = (int)rar + 1;
    for (int d = 0; d < dots; ++d) {
        SDL_SetRenderDrawColor(r, rr, rg, rb, 220);
        SDL_Rect dr = { rightX - (dots - d) * 9, y, 6, 6 };
        SDL_RenderFillRect(r, &dr);
    }
}

static void drawRarityDots(SDL_Renderer* r, ProgramRarity rar, int rightX, int y) {
    drawRarityDots(r, static_cast<ModRarity>(rar), rightX, y);
}

// ---------------------------------------------------------------------------
// Render — program selection
// ---------------------------------------------------------------------------
void LoadoutScene::render(SceneContext& ctx) {
    if (m_phase == Phase::StartingUpgrade) { renderUpgradePhase(ctx); return; }

    SDL_Renderer*   r  = ctx.renderer;
    VectorRenderer* vr = ctx.vrenderer;

    vr->clear();
    vr->drawGrid(20, 50, 80);
    vr->drawCRTOverlay();

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Build chart (right panel)
    float pulse = 0.5f + 0.5f * std::sin(m_pulse);
    renderBuildChart(r, ctx.hud, ctx.mods, ctx.programs, ctx.saveData,
                     pulse, CHART_X, CHART_Y, CHART_W, CHART_H);

    if (ctx.hud) {
        SDL_Color titleCol = { 0, 220, 180, 255 };
        SDL_Color subCol   = { 80, 160, 130, 200 };
        ctx.hud->drawLabel("// SELECT PROGRAMS //",
                           GRID_X, 22, titleCol);
        int picked = (int)m_picks.size();
        std::string sub = "PICK " + std::to_string(PICKS - picked) + " MORE";
        if (picked == PICKS) sub = "LOADING...";
        ctx.hud->drawLabel(sub.c_str(), GRID_X, 46, subCol);
    }

    for (int i = 0; i < POOL_SIZE; ++i)
        drawCard(ctx, i);

    if (ctx.hud) {
        SDL_Color hint = { 40, 120, 100, 180 };
        ctx.hud->drawLabel("SPACE/A to pick   TAB/START to confirm early",
                           GRID_X, Constants::SCREEN_H - 36, hint);
    }

    SDL_RenderPresent(r);
}

void LoadoutScene::drawCard(SceneContext& ctx, int idx) const {
    SDL_Renderer* r = ctx.renderer;

    int col = idx % COLS;
    int row = idx / COLS;
    int cx  = GRID_X + col * (CARD_W + CARD_PAD);
    int cy  = GRID_Y + row * (CARD_H + CARD_PAD);

    bool selected = std::find(m_picks.begin(), m_picks.end(), idx) != m_picks.end();
    bool hovering = (idx == m_cursor);
    float pulse   = 0.5f + 0.5f * std::sin(m_pulse);

    int pickNum = -1;
    for (int i = 0; i < (int)m_picks.size(); ++i)
        if (m_picks[i] == idx) { pickNum = i + 1; break; }

    const ProgramDef& def = getProgramDef(m_pool[idx]);

    // Ability type accent color
    uint8_t acR, acG, acB;
    abilityTypeColor(def.abilityType, acR, acG, acB);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Card background — dark, very slightly tinted by ability type
    uint8_t bgA = selected ? 230 : 185;
    SDL_SetRenderDrawColor(r, acR/22, acG/22, acB/22, bgA);
    SDL_Rect card = { cx, cy, CARD_W, CARD_H };
    SDL_RenderFillRect(r, &card);

    // Left accent stripe (4px, full height) — ability type color
    uint8_t stripeA = selected ? 255 : (hovering ? (uint8_t)(160 + 80*pulse) : 90);
    SDL_SetRenderDrawColor(r, acR, acG, acB, stripeA);
    SDL_Rect stripe = { cx, cy, 4, CARD_H };
    SDL_RenderFillRect(r, &stripe);

    // Border — ability type color
    if (selected) {
        SDL_SetRenderDrawColor(r, acR, acG, acB, 240);
        SDL_RenderDrawRect(r, &card);
        // Extra glow rect
        SDL_SetRenderDrawColor(r, acR, acG, acB, (uint8_t)(55 * pulse));
        SDL_Rect g = { cx-2, cy-2, CARD_W+4, CARD_H+4 };
        SDL_RenderDrawRect(r, &g);
    } else if (hovering) {
        SDL_SetRenderDrawColor(r, acR, acG, acB, (uint8_t)(120 + 100*pulse));
        SDL_RenderDrawRect(r, &card);
    } else {
        SDL_SetRenderDrawColor(r, acR/3, acG/3, acB/3, 160);
        SDL_RenderDrawRect(r, &card);
    }

    if (!ctx.hud) return;

    // Name (top, offset past stripe)
    SDL_Color nameCol = selected
        ? SDL_Color{ 255, 255, 255, 255 }
        : (hovering ? SDL_Color{ 230, 240, 235, 255 } : SDL_Color{ 170, 190, 180, 220 });
    ctx.hud->drawLabel(def.name, cx + 12, cy + 10, nameCol);

    // Ability type label (smaller, colored)
    SDL_Color typeCol = { acR, acG, acB, selected ? (uint8_t)200 : (uint8_t)140 };
    ctx.hud->drawLabel(abilityTypeName(def.abilityType), cx + 12, cy + 30, typeCol);

    // Description — wrap naively at ~22 chars
    SDL_Color descCol = { 120, 150, 140, 200 };
    std::string desc = def.desc;
    if ((int)desc.size() > 22) {
        size_t split = desc.rfind(' ', 22);
        if (split == std::string::npos) split = 22;
        ctx.hud->drawLabel(desc.substr(0, split).c_str(), cx + 12, cy + 50, descCol);
        ctx.hud->drawLabel(desc.substr(split + 1).c_str(), cx + 12, cy + 68, descCol);
    } else {
        ctx.hud->drawLabel(desc.c_str(), cx + 12, cy + 50, descCol);
    }

    // Cooldown (bottom-left)
    SDL_Color cdCol = { 150, 150, 90, 200 };
    std::string cdStr = "CD: " + std::to_string((int)def.cooldown) + "s";
    ctx.hud->drawLabel(cdStr.c_str(), cx + 12, cy + CARD_H - 20, cdCol);

    // Rarity dots (bottom-right)
    drawRarityDots(r, def.rarity, cx + CARD_W - 6, cy + CARD_H - 16);

    // Pick number badge (top-right)
    if (pickNum > 0) {
        SDL_SetRenderDrawColor(r, acR, acG, acB, 220);
        SDL_Rect badge = { cx + CARD_W - 22, cy + 4, 18, 18 };
        SDL_RenderFillRect(r, &badge);
        SDL_Color wht = { 240, 255, 240, 255 };
        ctx.hud->drawLabel(std::to_string(pickNum).c_str(), cx + CARD_W - 16, cy + 6, wht);
    }
}

// ---------------------------------------------------------------------------
// Starting upgrade phase
// ---------------------------------------------------------------------------

void LoadoutScene::renderUpgradePhase(SceneContext& ctx) {
    SDL_Renderer*   r  = ctx.renderer;
    VectorRenderer* vr = ctx.vrenderer;

    vr->clear();
    vr->drawGrid(20, 50, 80);
    vr->drawCRTOverlay();

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Build chart on the right (same region)
    float pulse = 0.5f + 0.5f * std::sin(m_pulse);
    renderBuildChart(r, ctx.hud, ctx.mods, ctx.programs, ctx.saveData,
                     pulse, CHART_X, CHART_Y - 30, CHART_W, CHART_H + 60);

    if (ctx.hud) {
        ctx.hud->drawLabel("// STARTING UPGRADE //",
                           UPG_X0, 22, { 0, 220, 180, 255 });
        ctx.hud->drawLabel("CHOOSE ONE  -  DEFINES YOUR BUILD",
                           UPG_X0, 46, { 80, 160, 130, 200 });
    }

    for (int i = 0; i < (int)m_modOffered.size(); ++i)
        drawModCard(ctx, i);

    SDL_RenderPresent(r);
}

void LoadoutScene::drawModCard(SceneContext& ctx, int idx) const {
    SDL_Renderer* r   = ctx.renderer;
    const ModDef& def = getModDef(m_modOffered[idx]);
    bool selected     = (idx == m_modCursor);
    float pulse       = 0.5f + 0.5f * std::sin(m_pulse);

    int cx = UPG_X0 + idx * (UPG_CARD_W + UPG_CARD_PAD);
    int cy = UPG_Y0;

    // Ability type accent color
    uint8_t acR, acG, acB;
    abilityTypeColor(def.abilityType, acR, acG, acB);

    // Rarity color (for border glow when selected)
    uint8_t rr, rg, rb;
    rarityColor(def.rarity, rr, rg, rb);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Card background
    SDL_SetRenderDrawColor(r, acR/20, acG/20, acB/20, selected ? 225 : 160);
    SDL_Rect card = { cx, cy, UPG_CARD_W, UPG_CARD_H };
    SDL_RenderFillRect(r, &card);

    // Left accent stripe
    uint8_t stripeA = selected ? 255 : 100;
    SDL_SetRenderDrawColor(r, acR, acG, acB, stripeA);
    SDL_Rect stripe = { cx, cy, 4, UPG_CARD_H };
    SDL_RenderFillRect(r, &stripe);

    // Border — rarity color when selected, dim ability color otherwise
    if (selected) {
        uint8_t ba = (uint8_t)(180 + 75 * pulse);
        SDL_SetRenderDrawColor(r, rr, rg, rb, ba);
        SDL_RenderDrawRect(r, &card);
        SDL_SetRenderDrawColor(r, rr, rg, rb, (uint8_t)(50 * pulse));
        SDL_Rect g1 = { cx-3, cy-3, UPG_CARD_W+6, UPG_CARD_H+6 }; SDL_RenderDrawRect(r, &g1);
        SDL_Rect g2 = { cx-2, cy-2, UPG_CARD_W+4, UPG_CARD_H+4 }; SDL_RenderDrawRect(r, &g2);
    } else {
        SDL_SetRenderDrawColor(r, acR/3, acG/3, acB/3, 170);
        SDL_RenderDrawRect(r, &card);
    }

    if (!ctx.hud) return;

    // Ability type label (top, colored)
    SDL_Color typeCol = { acR, acG, acB, selected ? (uint8_t)220 : (uint8_t)150 };
    ctx.hud->drawLabel(abilityTypeName(def.abilityType), cx + 12, cy + 12, typeCol);

    // Mod name (prominent)
    uint8_t nameA = selected ? 255 : 210;
    ctx.hud->drawLabel(def.name, cx + 12, cy + 34, { 255, 255, 255, nameA });

    // Category label (right of name, dim)
    const char* catLabel = "";
    switch (def.category) {
        case ModCategory::Chassis: catLabel = "CHASSIS"; break;
        case ModCategory::Weapon:  catLabel = "WEAPON";  break;
        case ModCategory::Neural:  catLabel = "NEURAL";  break;
    }
    ctx.hud->drawLabel(catLabel, cx + 12, cy + 56, { acR, acG, acB, 130 });

    // Description
    ctx.hud->drawLabel(def.desc, cx + 12, cy + 78, { 140, 160, 150, 200 });

    // Type label (Stat/Passive)
    const char* typeLabel = def.type == ModType::Stat ? "STAT (stackable)" : "PASSIVE";
    ctx.hud->drawLabel(typeLabel, cx + 12, cy + UPG_CARD_H - 44, { 90, 110, 100, 160 });

    // Rarity text + dots
    ctx.hud->drawLabel(rarityName(def.rarity), cx + 12, cy + UPG_CARD_H - 26,
                       { rr, rg, rb, nameA });
    drawRarityDots(r, def.rarity, cx + UPG_CARD_W - 6, cy + UPG_CARD_H - 22);

    if (selected)
        ctx.hud->drawLabel("[ ENTER / A TO CONFIRM ]",
                           cx + 12, cy + UPG_CARD_H - 8, { rr, rg, rb, 210 });
}
