#include "LoadoutScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "systems/ModSystem.hpp"
#include "core/Constants.hpp"
#include "renderer/HUD.hpp"
#include "renderer/VectorRenderer.hpp"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <random>

// ---------------------------------------------------------------------------
// Layout constants
// ---------------------------------------------------------------------------
static constexpr int COLS    = 3;
static constexpr int ROWS    = 2;
static constexpr int CARD_W  = 200;
static constexpr int CARD_H  = 110;
static constexpr int CARD_PAD = 18;

static constexpr int GRID_W = COLS * CARD_W + (COLS - 1) * CARD_PAD;
static constexpr int GRID_H = ROWS * CARD_H + (ROWS - 1) * CARD_PAD;
static constexpr int GRID_X = (Constants::SCREEN_W - GRID_W) / 2;
static constexpr int GRID_Y = (Constants::SCREEN_H - GRID_H) / 2 + 20;

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
// Pool generation — 6 unique programs drawn at random (CLONE excluded)
// ---------------------------------------------------------------------------
void LoadoutScene::buildPool() {
    // Available programs — skip CLONE (not implemented)
    std::vector<ProgramID> available;
    for (int i = 0; i < (int)ProgramID::COUNT; ++i) {
        ProgramID pid = static_cast<ProgramID>(i);
        available.push_back(pid);
    }
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
    // If already selected, deselect
    auto it = std::find(m_picks.begin(), m_picks.end(), m_cursor);
    if (it != m_picks.end()) {
        m_picks.erase(it);
        return;
    }
    // Can't pick more than PICKS
    if ((int)m_picks.size() >= PICKS) return;
    m_picks.push_back(m_cursor);

    // Auto-confirm once 3 are picked
    if ((int)m_picks.size() == PICKS) confirm(ctx);
}

void LoadoutScene::confirm(SceneContext& ctx) {
    if ((int)m_picks.size() < PICKS) return;
    for (int i = 0; i < PICKS; ++i)

    // First time entering combat: offer a starting upgrade before the fight
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
    ctx.scenes->replace(std::make_unique<CombatScene>(m_cfg));
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------
void LoadoutScene::update(float dt, SceneContext&) {
    m_pulse += dt * 3.f;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
void LoadoutScene::render(SceneContext& ctx) {
    if (m_phase == Phase::StartingUpgrade) { renderUpgradePhase(ctx); return; }

    SDL_Renderer*   r  = ctx.renderer;
    VectorRenderer* vr = ctx.vrenderer;

    vr->clear();
    vr->drawGrid(20, 50, 80);
    vr->drawCRTOverlay();

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Title
    if (ctx.hud) {
        SDL_Color titleCol  = { 0, 220, 180, 255 };
        SDL_Color subCol    = { 80, 160, 130, 200 };
        ctx.hud->drawLabel("// SELECT PROGRAMS //",
                           Constants::SCREEN_W / 2 - 145, 22, titleCol);
        int picked = (int)m_picks.size();
        std::string sub = "PICK " + std::to_string(PICKS - picked) + " MORE";
        if (picked == PICKS) sub = "LOADING...";
        ctx.hud->drawLabel(sub, Constants::SCREEN_W / 2 - 60, 52, subCol);
    }

    // Draw all 6 cards
    for (int i = 0; i < POOL_SIZE; ++i)
        drawCard(ctx, i);

    // Bottom hint
    if (ctx.hud) {
        SDL_Color hint = { 40, 120, 100, 180 };
        ctx.hud->drawLabel("SPACE/A to pick   TAB/START to confirm early",
                           Constants::SCREEN_W / 2 - 230, Constants::SCREEN_H - 36, hint);
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

    // Pick order badge (1/2/3)
    int pickNum = -1;
    for (int i = 0; i < (int)m_picks.size(); ++i)
        if (m_picks[i] == idx) { pickNum = i + 1; break; }

    // Card background
    uint8_t bgR = selected ? 5  : 2;
    uint8_t bgG = selected ? 40 : 8;
    uint8_t bgB = selected ? 30 : 20;
    uint8_t bgA = selected ? 230 : 180;
    SDL_SetRenderDrawColor(r, bgR, bgG, bgB, bgA);
    SDL_Rect card = { cx, cy, CARD_W, CARD_H };
    SDL_RenderFillRect(r, &card);

    // Border
    if (selected) {
        SDL_SetRenderDrawColor(r, 0, 220, 160, 240);
    } else if (hovering) {
        auto alpha = static_cast<uint8_t>(120 + 120 * pulse);
        SDL_SetRenderDrawColor(r, 20, 180, 150, alpha);
    } else {
        SDL_SetRenderDrawColor(r, 30, 80, 70, 160);
    }
    SDL_RenderDrawRect(r, &card);

    // Hover glow rect
    if (hovering && !selected) {
        SDL_SetRenderDrawColor(r, 0, 140, 120,
            static_cast<uint8_t>(30 * pulse));
        SDL_Rect glow = { cx - 2, cy - 2, CARD_W + 4, CARD_H + 4 };
        SDL_RenderDrawRect(r, &glow);
    }

    // Selected: inner accent line at top
    if (selected) {
        SDL_SetRenderDrawColor(r, 0, 200, 140, 180);
        SDL_RenderDrawLine(r, cx + 3, cy + 3, cx + CARD_W - 3, cy + 3);
    }

    if (!ctx.hud) return;

    const ProgramDef& def = getProgramDef(m_pool[idx]);

    SDL_Color nameCol = selected
        ? SDL_Color{ 0, 240, 180, 255 }
        : (hovering ? SDL_Color{ 180, 240, 220, 255 } : SDL_Color{ 120, 180, 160, 220 });
    SDL_Color descCol = { 100, 160, 140, 200 };
    SDL_Color cdCol   = { 160, 160, 100, 200 };

    ctx.hud->drawLabel(def.name, cx + 10, cy + 12, nameCol);

    // Description — wrap naively at ~22 chars
    std::string desc = def.desc;
    if ((int)desc.size() > 22) {
        // Find last space before char 22
        size_t split = desc.rfind(' ', 22);
        if (split == std::string::npos) split = 22;
        ctx.hud->drawLabel(desc.substr(0, split), cx + 10, cy + 42, descCol);
        ctx.hud->drawLabel(desc.substr(split + 1), cx + 10, cy + 62, descCol);
    } else {
        ctx.hud->drawLabel(desc, cx + 10, cy + 42, descCol);
    }

    // Cooldown
    std::string cdStr = "CD: " + std::to_string((int)def.cooldown) + "s";
    ctx.hud->drawLabel(cdStr, cx + 10, cy + 84, cdCol);

    // Pick number badge (top-right corner)
    if (pickNum > 0) {
        SDL_SetRenderDrawColor(r, 0, 180, 130, 220);
        SDL_Rect badge = { cx + CARD_W - 22, cy + 4, 18, 18 };
        SDL_RenderFillRect(r, &badge);
        SDL_Color wht = { 240, 255, 240, 255 };
        ctx.hud->drawLabel(std::to_string(pickNum), cx + CARD_W - 16, cy + 6, wht);
    }
}

// ---------------------------------------------------------------------------
// Starting upgrade phase
// ---------------------------------------------------------------------------

static constexpr int UPG_CARD_W   = 260;
static constexpr int UPG_CARD_H   = 160;
static constexpr int UPG_CARD_PAD = 20;
static constexpr int UPG_TOTAL    = LoadoutScene::MOD_OFFERS * UPG_CARD_W
                                  + (LoadoutScene::MOD_OFFERS - 1) * UPG_CARD_PAD;
static constexpr int UPG_X0       = (Constants::SCREEN_W - UPG_TOTAL) / 2;
static constexpr int UPG_Y0       = (Constants::SCREEN_H - UPG_CARD_H) / 2 + 30;

void LoadoutScene::renderUpgradePhase(SceneContext& ctx) {
    ctx.vrenderer->clear();
    ctx.vrenderer->drawGrid(20, 50, 80);
    ctx.vrenderer->drawCRTOverlay();

    if (ctx.hud) {
        ctx.hud->drawLabel("// STARTING UPGRADE //",
                           Constants::SCREEN_W / 2 - 145, 22, { 0, 220, 180, 255 });
        ctx.hud->drawLabel("CHOOSE ONE  -  DEFINES YOUR BUILD",
                           Constants::SCREEN_W / 2 - 185, 52, { 80, 160, 130, 200 });
    }

    for (int i = 0; i < (int)m_modOffered.size(); ++i)
        drawModCard(ctx, i);

    SDL_RenderPresent(ctx.renderer);
}

void LoadoutScene::drawModCard(SceneContext& ctx, int idx) const {
    SDL_Renderer* r   = ctx.renderer;
    const ModDef& def = getModDef(m_modOffered[idx]);
    bool selected     = (idx == m_modCursor);
    float pulse       = 0.5f + 0.5f * std::sin(m_pulse);

    int cx = UPG_X0 + idx * (UPG_CARD_W + UPG_CARD_PAD);
    int cy = UPG_Y0;

    uint8_t cr, cg, cb;
    switch (def.category) {
        case ModCategory::Chassis: cr=40;  cg=210; cb=120; break;
        case ModCategory::Weapon:  cr=230; cg=180; cb=30;  break;
        case ModCategory::Neural:  cr=180; cg=60;  cb=255; break;
        default:                   cr=100; cg=100; cb=100; break;
    }
    uint8_t rr, rg, rb;
    rarityColor(def.rarity, rr, rg, rb);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, rr/14, rg/14, rb/14, selected ? 220 : 150);
    SDL_Rect card = { cx, cy, UPG_CARD_W, UPG_CARD_H };
    SDL_RenderFillRect(r, &card);

    if (selected) {
        uint8_t alpha = (uint8_t)(180 + 75 * pulse);
        SDL_SetRenderDrawColor(r, rr, rg, rb, alpha);
        SDL_RenderDrawRect(r, &card);
        SDL_SetRenderDrawColor(r, rr, rg, rb, (uint8_t)(50 * pulse));
        SDL_Rect g1 = { cx-3, cy-3, UPG_CARD_W+6, UPG_CARD_H+6 }; SDL_RenderDrawRect(r, &g1);
        SDL_Rect g2 = { cx-2, cy-2, UPG_CARD_W+4, UPG_CARD_H+4 }; SDL_RenderDrawRect(r, &g2);
    } else {
        SDL_SetRenderDrawColor(r, rr/2, rg/2, rb/2, 180);
        SDL_RenderDrawRect(r, &card);
    }

    SDL_SetRenderDrawColor(r, cr, cg, cb, selected ? 200 : 80);
    SDL_RenderDrawLine(r, cx+4, cy+4, cx+UPG_CARD_W-4, cy+4);
    SDL_RenderDrawLine(r, cx+4, cy+5, cx+UPG_CARD_W-4, cy+5);

    if (!ctx.hud) return;

    const char* catLabel = "";
    switch (def.category) {
        case ModCategory::Chassis: catLabel = "CHASSIS"; break;
        case ModCategory::Weapon:  catLabel = "WEAPON";  break;
        case ModCategory::Neural:  catLabel = "NEURAL";  break;
    }
    uint8_t nameAlpha = selected ? 255 : 200;
    ctx.hud->drawLabel(catLabel,               cx+10, cy+14, { cr, cg, cb, nameAlpha });
    ctx.hud->drawLabel(def.name,               cx+10, cy+46, { 255, 255, 255, nameAlpha });
    ctx.hud->drawLabel(def.desc,               cx+10, cy+80, { 160, 180, 170, 200 });
    ctx.hud->drawLabel(rarityName(def.rarity), cx+10, cy+UPG_CARD_H-50, { rr, rg, rb, nameAlpha });

    if (selected)
        ctx.hud->drawLabel("[ CONFIRM ]", cx+10, cy+UPG_CARD_H-30, { rr, rg, rb, 200 });
}
