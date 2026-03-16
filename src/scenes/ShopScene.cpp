#include "ShopScene.hpp"
#include "MainMenuScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/Constants.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include <SDL.h>
#include <cmath>
#include <string>
#include <algorithm>
#include <vector>

// ---------------------------------------------------------------------------
// Shop catalog — upgrades only (hulls are unlocked via gameplay)
// ---------------------------------------------------------------------------

struct ShopItem {
    const char* id;
    const char* name;
    const char* desc;
    int         cost;
    int         maxPurchases;
};

static const ShopItem SHOP_CATALOG[] = {
    // ----- Permanent upgrades -----
    {"EXTRA_LIFE",    "MILITARY CHASSIS",   "Start each run with +1 extra life",      800, 3},
    {"EXTRA_OFFER",   "NEURAL BUFFER",      "+1 upgrade card offered per pick",       1200, 2},
    {"TRACE_SLOW",    "TRACE DAMPENER",     "Trace ticks 10% slower permanently",     600, 3},
    {"START_PROGRAM", "PROGRAM INJECTOR",   "Start each run with 1 random program",   1000, 1},
    {"SCORE_BOOST",   "DATA SIPHON",        "All score gains +20%",                   900, 3},
    {"EXTRA_PASSIVE", "NEURAL HARDWIRE",    "Passive mod slot limit +1 (max 5)",      2000, 2},
    {"COOL_EXEC",     "CLOCK CRYSTAL",      "All program cooldowns -10% permanently", 500, 4},
    {"REVEAL_MAP",    "SHADOW SCANNER",     "Reveals all node names and types on map (still must unlock normally)", 1500, 1},
    // ----- Legendary -----
    {"DEADBOLT",      "DEADBOLT PROTOCOL",  "Your own bullets can never harm you. Friendly fire immunity, permanently.", 8000, 1},
};
static constexpr int CATALOG_SIZE = (int)(sizeof(SHOP_CATALOG) / sizeof(SHOP_CATALOG[0]));

// ---------------------------------------------------------------------------
// Vector icons — drawn procedurally per item
// ---------------------------------------------------------------------------

static void drawCircle(SDL_Renderer* r, int cx, int cy, int rad, int segs = 16) {
    const float PI = 3.14159f;
    for (int i = 0; i < segs; ++i) {
        float a1 = i * 2*PI/segs, a2 = (i+1) * 2*PI/segs;
        SDL_RenderDrawLine(r,
            cx + (int)(rad * cosf(a1)), cy + (int)(rad * sinf(a1)),
            cx + (int)(rad * cosf(a2)), cy + (int)(rad * sinf(a2)));
    }
}

static void drawShopIcon(SDL_Renderer* r, const char* id, int cx, int cy, int sz) {
    int h = sz / 2;
    const float PI = 3.14159f;

    {
        if (id == std::string("EXTRA_LIFE")) {
            // Shield: pentagon
            int px[5], py[5];
            for (int i = 0; i < 5; ++i) {
                float a = -PI/2 + i * 2*PI/5;
                px[i] = cx + (int)(h * cosf(a));
                py[i] = cy + (int)(h * sinf(a));
            }
            for (int i = 0; i < 5; ++i)
                SDL_RenderDrawLine(r, px[i], py[i], px[(i+1)%5], py[(i+1)%5]);
            // Inner line for shield face
            SDL_RenderDrawLine(r, px[0], py[0], cx, cy + h/3);

        } else if (id == std::string("EXTRA_OFFER")) {
            // Neural net: 3 nodes connected
            SDL_RenderDrawLine(r, cx,        cy - h,    cx - h/2, cy + h/2);
            SDL_RenderDrawLine(r, cx,        cy - h,    cx + h/2, cy + h/2);
            SDL_RenderDrawLine(r, cx - h/2,  cy + h/2,  cx + h/2, cy + h/2);
            SDL_RenderDrawLine(r, cx - h/2,  cy + h/2,  cx,       cy - h/4);
            SDL_RenderDrawLine(r, cx + h/2,  cy + h/2,  cx,       cy - h/4);
            SDL_Rect n1 = {cx-2,      cy-h-2,    5, 5};
            SDL_Rect n2 = {cx-h/2-2,  cy+h/2-2,  5, 5};
            SDL_Rect n3 = {cx+h/2-2,  cy+h/2-2,  5, 5};
            SDL_RenderFillRect(r, &n1);
            SDL_RenderFillRect(r, &n2);
            SDL_RenderFillRect(r, &n3);

        } else if (id == std::string("TRACE_SLOW")) {
            // Dampening sine wave
            for (int x = -h; x < h - 1; ++x) {
                float amp = h * 0.45f * (1.f - fabsf((float)x / h) * 0.4f);
                float t1 = (float)x     / h * 2 * PI;
                float t2 = (float)(x+1) / h * 2 * PI;
                SDL_RenderDrawLine(r,
                    cx + x,   cy + (int)(amp * sinf(t1)),
                    cx + x+1, cy + (int)(amp * sinf(t2)));
            }

        } else if (id == std::string("START_PROGRAM")) {
            // Program card: rounded rect with a bolt inside
            SDL_Rect card = {cx - h/2, cy - h*2/3, h, h*4/3};
            SDL_RenderDrawRect(r, &card);
            // Lightning bolt
            SDL_RenderDrawLine(r, cx + h/6, cy - h/3, cx - h/6, cy);
            SDL_RenderDrawLine(r, cx - h/6, cy,        cx + h/6, cy + h/3);

        } else if (id == std::string("SCORE_BOOST")) {
            // Upward arrow with data ticks
            SDL_RenderDrawLine(r, cx, cy - h,    cx, cy + h/2);
            SDL_RenderDrawLine(r, cx, cy - h,    cx - h/2, cy - h/3);
            SDL_RenderDrawLine(r, cx, cy - h,    cx + h/2, cy - h/3);
            // Tick marks
            for (int i = -1; i <= 1; i += 2) {
                SDL_RenderDrawLine(r, cx + i*h/2, cy - h/3, cx + i*h/2, cy + h/3);
            }

        } else if (id == std::string("EXTRA_PASSIVE")) {
            // Stacked horizontal bars (layers/stack)
            for (int i = -2; i <= 2; ++i) {
                int ly  = cy + i * (h * 2 / 5);
                int lw  = h - abs(i) * h / 4;
                SDL_Rect bar = {cx - lw, ly - 2, lw * 2, 4};
                SDL_RenderDrawRect(r, &bar);
            }

        } else if (id == std::string("COOL_EXEC")) {
            // Hexagonal crystal
            int px[6], py[6];
            for (int i = 0; i < 6; ++i) {
                float a = i * 2*PI/6;
                px[i] = cx + (int)(h * cosf(a));
                py[i] = cy + (int)(h * sinf(a));
            }
            for (int i = 0; i < 6; ++i)
                SDL_RenderDrawLine(r, px[i], py[i], px[(i+1)%6], py[(i+1)%6]);
            // Inner star lines
            SDL_RenderDrawLine(r, px[0], py[0], px[3], py[3]);
            SDL_RenderDrawLine(r, px[1], py[1], px[4], py[4]);
            SDL_RenderDrawLine(r, px[2], py[2], px[5], py[5]);

        } else if (id == std::string("REVEAL_MAP")) {
            // Radar: concentric rings + sweep arm
            for (int rad = h/3; rad <= h; rad += h/3)
                drawCircle(r, cx, cy, rad, 20);
            // Sweep arm
            SDL_RenderDrawLine(r, cx, cy, cx + (int)(h * 0.85f), cy - h/3);
            // Centre dot
            SDL_Rect dot = {cx - 2, cy - 2, 5, 5};
            SDL_RenderFillRect(r, &dot);
        }
    }
}


// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ShopScene::onEnter(SceneContext&) {
    m_cursor      = 0;
    m_time        = 0.f;
    m_pulse       = 0.f;
    m_statusMsg   = "";
    m_statusTimer = 0.f;
}

void ShopScene::onExit() {}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void ShopScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    int total = CATALOG_SIZE;
    auto move = [&](int d) {
        m_cursor = (m_cursor + d + total) % total;
    };

    if (ev.type == SDL_KEYDOWN) {
        const auto sc = ev.key.keysym.scancode;
        if      (sc == SDL_SCANCODE_UP   || sc == SDL_SCANCODE_W) move(-1);
        else if (sc == SDL_SCANCODE_DOWN || sc == SDL_SCANCODE_S) move(+1);
        else if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_SPACE) tryBuy(ctx);
        else if (sc == SDL_SCANCODE_ESCAPE || sc == SDL_SCANCODE_BACKSPACE)
            ctx.scenes->replace(std::make_unique<MainMenuScene>());
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        const auto btn = ev.cbutton.button;
        if      (btn == SDL_CONTROLLER_BUTTON_DPAD_UP)   move(-1);
        else if (btn == SDL_CONTROLLER_BUTTON_DPAD_DOWN) move(+1);
        else if (btn == SDL_CONTROLLER_BUTTON_A)         tryBuy(ctx);
        else if (btn == SDL_CONTROLLER_BUTTON_B || btn == SDL_CONTROLLER_BUTTON_START)
            ctx.scenes->replace(std::make_unique<MainMenuScene>());
    }
}

void ShopScene::tryBuy(SceneContext& ctx) {
    if (!ctx.saveData) {
        m_statusMsg = "NO SAVE DATA"; m_statusTimer = 2.f; return;
    }
    const ShopItem& item  = SHOP_CATALOG[m_cursor];
    int             owned = ctx.saveData->purchaseCount(item.id);

    if (owned >= item.maxPurchases) {
        m_statusMsg = "MAX PURCHASES REACHED"; m_statusTimer = 2.f; return;
    }
    if (ctx.saveData->credits < item.cost) {
        m_statusMsg = "INSUFFICIENT CREDITS"; m_statusTimer = 2.f; return;
    }

    ctx.saveData->credits -= item.cost;
    ctx.saveData->purchases.push_back(item.id);
    SaveSystem::save(*ctx.saveData);
    m_statusMsg = std::string(item.name) + " ACQUIRED";
    m_statusTimer = 2.f;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void ShopScene::update(float dt, SceneContext&) {
    m_time  += dt;
    m_pulse += dt * 3.f;
    if (m_statusTimer > 0.f) m_statusTimer -= dt;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void ShopScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;

    vr->clear();
    vr->drawGrid(10, 50, 80);
    vr->drawCRTOverlay();

    if (!ctx.hud) { SDL_RenderPresent(r); return; }

    // ---- Header ----
    std::string title = "// WAREZ SHOP //";
    int titleW = ctx.hud->measureText(title);
    ctx.hud->drawLabel(title, Constants::SCREEN_W / 2 - titleW / 2, 14, {0, 220, 180, 255});

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 180, 140, 80);
    SDL_RenderDrawLine(r, 20, 42, Constants::SCREEN_W - 20, 42);

    if (ctx.saveData) {
        std::string credStr = "CREDITS: " + std::to_string(ctx.saveData->credits) + " CR";
        int cw = ctx.hud->measureText(credStr);
        ctx.hud->drawLabel(credStr, Constants::SCREEN_W - cw - 20, 14, {0, 220, 160, 255});
    }

    // ---- Layout constants ----
    constexpr int PANEL_Y  = 50;
    constexpr int PANEL_H  = Constants::SCREEN_H - PANEL_Y - 32;
    constexpr int LIST_X   = 20;
    constexpr int LIST_W   = 490;
    constexpr int DIVIDER_X = LIST_X + LIST_W + 14;
    constexpr int DETAIL_X = DIVIDER_X + 14;
    constexpr int DETAIL_W = Constants::SCREEN_W - DETAIL_X - 20;
    constexpr int ITEM_H   = 44;
    constexpr int ICON_SZ  = 26;

    // Vertical divider
    SDL_SetRenderDrawColor(r, 0, 100, 80, 80);
    SDL_RenderDrawLine(r, DIVIDER_X, PANEL_Y, DIVIDER_X, PANEL_Y + PANEL_H);

    // ---- Left panel: item list ----
    int listY = PANEL_Y + 2;

    for (int i = 0; i < CATALOG_SIZE; ++i) {
        // Section headers
        if (i == 0) {
            ctx.hud->drawLabel("// PERMANENT UPGRADES", LIST_X, listY, {60, 130, 100, 160});
            listY += 26;
        } else if (i == 8) {
            listY += 6;
            ctx.hud->drawLabel("// LEGENDARY", LIST_X, listY, {200, 160, 20, 200});
            listY += 26;
        }

        const ShopItem& item = SHOP_CATALOG[i];
        bool sel    = (i == m_cursor);
        float pulse = sel ? (0.5f + 0.5f * std::sin(m_pulse)) : 0.f;
        int owned   = ctx.saveData ? ctx.saveData->purchaseCount(item.id) : 0;
        bool maxed  = (owned >= item.maxPurchases);
        bool afford = ctx.saveData && ctx.saveData->credits >= item.cost;

        // Row background
        SDL_Rect row = {LIST_X, listY, LIST_W, ITEM_H - 2};
        if (sel) {
            SDL_SetRenderDrawColor(r, 0, 45, 36, 210);
            SDL_RenderFillRect(r, &row);
            uint8_t ba = (uint8_t)(150 + (int)(105 * pulse));
            SDL_SetRenderDrawColor(r, 0, ba, (uint8_t)(ba * 0.8f), 220);
            SDL_RenderDrawRect(r, &row);
        } else {
            SDL_SetRenderDrawColor(r, 0, 20, 16, 120);
            SDL_RenderFillRect(r, &row);
            SDL_SetRenderDrawColor(r, 0, 50, 40, 80);
            SDL_RenderDrawRect(r, &row);
        }

        // Small icon
        int iconCX = LIST_X + 4 + ICON_SZ / 2;
        int iconCY = listY + ITEM_H / 2 - 1;
        SDL_SetRenderDrawColor(r, 0, sel ? 220 : 180, sel ? 200 : 140, sel ? 230 : 150);
        drawShopIcon(r, item.id, iconCX, iconCY, ICON_SZ);

        // Name
        SDL_Color nameCol;
        if      (maxed)           nameCol = {70, 110, 80, 180};
        else if (sel)             nameCol = {220, 240, 220, 255};
        else if (!afford)         nameCol = {160, 120, 80, 200};
        else                      nameCol = {180, 200, 190, 210};
        ctx.hud->drawLabel(item.name, LIST_X + ICON_SZ + 12, listY + 4, nameCol);

        // Upgrade sub-line
        std::string kindStr = "Upgrade";
        if (item.maxPurchases > 1)
            kindStr += "  " + std::to_string(owned) + "/" + std::to_string(item.maxPurchases);
        ctx.hud->drawLabel(kindStr, LIST_X + ICON_SZ + 12, listY + 24, {40, 120, 90, 150});

        // Cost — right-aligned
        std::string costStr = maxed ? "MAXED" : std::to_string(item.cost) + "CR";
        SDL_Color costCol = maxed ? SDL_Color{50, 100, 60, 150}
                                  : (afford ? SDL_Color{0, 210, 160, 220} : SDL_Color{210, 80, 50, 200});
        int cw2 = ctx.hud->measureText(costStr);
        ctx.hud->drawLabel(costStr, LIST_X + LIST_W - cw2 - 8, listY + 4, costCol);

        listY += ITEM_H;
    }

    // ---- Right panel: selected item detail ----
    const ShopItem& sel   = SHOP_CATALOG[m_cursor];
    int owned_sel   = ctx.saveData ? ctx.saveData->purchaseCount(sel.id) : 0;
    bool maxed_sel  = (owned_sel >= sel.maxPurchases);
    bool afford_sel = ctx.saveData && ctx.saveData->credits >= sel.cost;
    float pulse_sel = 0.5f + 0.5f * std::sin(m_pulse);

    // Panel background
    SDL_SetRenderDrawColor(r, 0, 14, 10, 200);
    SDL_Rect detBg = {DETAIL_X, PANEL_Y, DETAIL_W, PANEL_H};
    SDL_RenderFillRect(r, &detBg);
    SDL_SetRenderDrawColor(r, 0, 100, 80, 100);
    SDL_RenderDrawRect(r, &detBg);

    // Large icon
    constexpr int LARGE_ICON = 80;
    int iconCX = DETAIL_X + DETAIL_W / 2;
    int iconCY = PANEL_Y + LARGE_ICON / 2 + 24;
    SDL_SetRenderDrawColor(r, 0, (uint8_t)(200 + (int)(20*pulse_sel)), (uint8_t)(170 + (int)(30*pulse_sel)), 220);
    drawShopIcon(r, sel.id, iconCX, iconCY, LARGE_ICON);

    int detY = PANEL_Y + LARGE_ICON + 40;

    // Item name centred
    int nmw = ctx.hud->measureText(sel.name);
    ctx.hud->drawLabel(sel.name, DETAIL_X + DETAIL_W/2 - nmw/2, detY, {220, 240, 220, 255});
    detY += 28;

    // Category badge
    std::string badge;
    SDL_Color   badgeCol;
    if (std::string(sel.id) == "DEADBOLT") {
        badge = "[ LEGENDARY ]"; badgeCol = {220, 180, 20, 230};
    } else {
        badge = "[ PERMANENT UPGRADE ]"; badgeCol = {0, 200, 160, 200};
    }
    int bw = ctx.hud->measureText(badge);
    ctx.hud->drawLabel(badge, DETAIL_X + DETAIL_W/2 - bw/2, detY, badgeCol);
    detY += 30;

    // Separator
    SDL_SetRenderDrawColor(r, 0, 90, 70, 80);
    SDL_RenderDrawLine(r, DETAIL_X + 16, detY, DETAIL_X + DETAIL_W - 16, detY);
    detY += 14;

    // Full description, word-wrapped
    int linesDrawn = ctx.hud->drawWrapped(sel.desc,
        DETAIL_X + 16, detY, DETAIL_W - 32, {140, 190, 160, 220}, 24);
    detY += linesDrawn * 24 + 18;

    // Separator
    SDL_SetRenderDrawColor(r, 0, 90, 70, 80);
    SDL_RenderDrawLine(r, DETAIL_X + 16, detY, DETAIL_X + DETAIL_W - 16, detY);
    detY += 14;

    // Cost / status
    std::string costDisp;
    SDL_Color   costDispCol;
    if (maxed_sel) {
        costDisp    = "// MAXED //";
        costDispCol = {80, 140, 90, 200};
    } else {
        costDisp    = "COST: " + std::to_string(sel.cost) + " CR";
        costDispCol = afford_sel ? SDL_Color{0, 220, 160, 255} : SDL_Color{220, 80, 50, 220};
    }
    int cdw = ctx.hud->measureText(costDisp);
    ctx.hud->drawLabel(costDisp, DETAIL_X + DETAIL_W/2 - cdw/2, detY, costDispCol);
    detY += 28;

    if (sel.maxPurchases > 1) {
        std::string ownStr = "PURCHASED: " + std::to_string(owned_sel)
                           + " / " + std::to_string(sel.maxPurchases);
        int ow = ctx.hud->measureText(ownStr);
        ctx.hud->drawLabel(ownStr, DETAIL_X + DETAIL_W/2 - ow/2, detY, {80, 130, 100, 180});
        detY += 28;
    }

    // Buy hint
    if (!maxed_sel) {
        std::string hint    = afford_sel ? "[ ENTER ] to purchase" : "// INSUFFICIENT CREDITS //";
        SDL_Color   hintCol = afford_sel ? SDL_Color{0, 180, 140, 200} : SDL_Color{180, 60, 40, 180};
        int hw = ctx.hud->measureText(hint);
        ctx.hud->drawLabel(hint, DETAIL_X + DETAIL_W/2 - hw/2, detY, hintCol);
    }

    // Status flash
    if (m_statusTimer > 0.f) {
        float fade = std::min(1.f, m_statusTimer);
        int sw = ctx.hud->measureText(m_statusMsg);
        ctx.hud->drawLabel(m_statusMsg,
                           Constants::SCREEN_W / 2 - sw / 2,
                           Constants::SCREEN_H - 54,
                           {0, 220, 160, (uint8_t)(200 * fade)});
    }

    // Footer
    std::string footer = "UP/DOWN: Navigate    ENTER: Buy    ESC/B: Back";
    int fw = ctx.hud->measureText(footer);
    ctx.hud->drawLabel(footer,
                       Constants::SCREEN_W / 2 - fw / 2,
                       Constants::SCREEN_H - 24,
                       {60, 100, 80, 180});

    SDL_RenderPresent(r);
}
