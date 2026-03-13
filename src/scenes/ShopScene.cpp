#include "ShopScene.hpp"
#include "MainMenuScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/Constants.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "entities/Avatar.hpp"

#include <SDL.h>
#include <cmath>
#include <string>
#include <algorithm>
#include <vector>

// ---------------------------------------------------------------------------
// Shop catalog — upgrades + hull skins
// ---------------------------------------------------------------------------

enum class ItemKind { Upgrade, Hull };

struct ShopItem {
    const char* id;
    const char* name;
    const char* desc;
    int         cost;
    int         maxPurchases;
    ItemKind    kind;
};

static const ShopItem SHOP_CATALOG[] = {
    // ----- Permanent upgrades -----
    {"EXTRA_LIFE",    "MILITARY CHASSIS",   "Start each run with +1 extra life",      800, 3, ItemKind::Upgrade},
    {"EXTRA_OFFER",   "NEURAL BUFFER",      "+1 upgrade card offered per pick",       1200, 2, ItemKind::Upgrade},
    {"TRACE_SLOW",    "TRACE DAMPENER",     "Trace ticks 10% slower permanently",     600, 3, ItemKind::Upgrade},
    {"START_PROGRAM", "PROGRAM INJECTOR",   "Start each run with 1 random program",   1000, 1, ItemKind::Upgrade},
    {"SCORE_BOOST",   "DATA SIPHON",        "All score gains +20%",                   900, 3, ItemKind::Upgrade},
    {"EXTRA_PASSIVE", "NEURAL HARDWIRE",    "Passive mod slot limit +1 (max 5)",      2000, 2, ItemKind::Upgrade},
    {"COOL_EXEC",     "CLOCK CRYSTAL",      "All program cooldowns -10% permanently", 500, 4, ItemKind::Upgrade},
    {"REVEAL_MAP",    "SHADOW SCANNER",     "Reveals all node names and types on map (still must unlock normally)", 1500, 1, ItemKind::Upgrade},
    // ----- Legendary -----
    {"DEADBOLT",      "DEADBOLT PROTOCOL",  "Your own bullets can never harm you. Friendly fire immunity, permanently.", 8000, 1, ItemKind::Upgrade},
    // ----- Hull skins (cosmetic) -----
    {"HULL_RAPTOR",   "SIGNAL KNIFE",       "Swept-wing interceptor. Fast. Fragile.",  600, 1, ItemKind::Hull},
    {"HULL_MANTIS",   "STRIKE FRAME",       "Forward-claw gunship. Hits hard.",        600, 1, ItemKind::Hull},
    {"HULL_BLADE",    "GHOST WIRE",         "Knife silhouette. Kills in one pass.",    600, 1, ItemKind::Hull},
    {"HULL_BATTLE",   "IRON COFFIN",        "Heavy armored delta. Slow. Survives.",    600, 1, ItemKind::Hull},
};
static constexpr int CATALOG_SIZE = (int)(sizeof(SHOP_CATALOG) / sizeof(SHOP_CATALOG[0]));

// Map hull item id → activeHull string
static const char* hullIdToActive(const char* id) {
    if (id == std::string("HULL_RAPTOR"))  return "RAPTOR";
    if (id == std::string("HULL_MANTIS"))  return "MANTIS";
    if (id == std::string("HULL_BLADE"))   return "BLADE";
    if (id == std::string("HULL_BATTLE"))  return "BATTLE";
    return "DELTA";
}

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

static void drawShopIcon(SDL_Renderer* r, const char* id, ItemKind kind, int cx, int cy, int sz) {
    int h = sz / 2;
    const float PI = 3.14159f;

    if (kind == ItemKind::Hull) {
        // Use the real in-game hull geometry (same function as Avatar + ShipSelectScene)
        HullType ht = HullType::Delta;
        if      (id == std::string("HULL_RAPTOR")) ht = HullType::Raptor;
        else if (id == std::string("HULL_MANTIS")) ht = HullType::Mantis;
        else if (id == std::string("HULL_BLADE"))  ht = HullType::Blade;
        else if (id == std::string("HULL_BATTLE")) ht = HullType::Battle;

        auto verts = hullVerts(ht);
        // Natural bounding box spans ~80 units in Y — scale to fit sz
        float scale = sz / 80.f;
        std::vector<SDL_Point> pts;
        pts.reserve(verts.size() + 1);
        for (auto& v : verts)
            pts.push_back({cx + (int)(v.x * scale), cy + (int)(v.y * scale)});
        pts.push_back(pts.front()); // close polygon
        SDL_RenderDrawLines(r, pts.data(), (int)pts.size());
    } else {
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
        // For hulls, if already owned, just equip it
        if (item.kind == ItemKind::Hull && owned > 0) {
            ctx.saveData->activeHull = hullIdToActive(item.id);
            SaveSystem::save(*ctx.saveData);
            m_statusMsg = std::string(item.name) + " EQUIPPED"; m_statusTimer = 2.f;
            return;
        }
        m_statusMsg = "MAX PURCHASES REACHED"; m_statusTimer = 2.f; return;
    }
    if (ctx.saveData->credits < item.cost) {
        m_statusMsg = "INSUFFICIENT CREDITS"; m_statusTimer = 2.f; return;
    }

    ctx.saveData->credits -= item.cost;
    ctx.saveData->purchases.push_back(item.id);

    if (item.kind == ItemKind::Hull)
        ctx.saveData->activeHull = hullIdToActive(item.id);

    SaveSystem::save(*ctx.saveData);
    m_statusMsg = std::string(item.name) + (item.kind == ItemKind::Hull ? " EQUIPPED" : " ACQUIRED");
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

        std::string hullStr = "HULL: " + ctx.saveData->activeHull;
        ctx.hud->drawLabel(hullStr, 20, 14, {80, 160, 220, 200});
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
        } else if (i == 9) {
            listY += 6;
            ctx.hud->drawLabel("// SHIP HULLS", LIST_X, listY, {80, 120, 180, 160});
            listY += 26;
        }

        const ShopItem& item = SHOP_CATALOG[i];
        bool sel    = (i == m_cursor);
        float pulse = sel ? (0.5f + 0.5f * std::sin(m_pulse)) : 0.f;
        int owned   = ctx.saveData ? ctx.saveData->purchaseCount(item.id) : 0;
        bool maxed  = (owned >= item.maxPurchases);
        bool afford = ctx.saveData && ctx.saveData->credits >= item.cost;
        bool hullActive = (item.kind == ItemKind::Hull && ctx.saveData &&
                           ctx.saveData->activeHull == hullIdToActive(item.id));

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

        if (hullActive) {
            SDL_SetRenderDrawColor(r, 0, 200, 160, 40);
            SDL_Rect act = {LIST_X+2, listY+2, LIST_W-4, ITEM_H-6};
            SDL_RenderFillRect(r, &act);
        }

        // Small icon
        int iconCX = LIST_X + 4 + ICON_SZ / 2;
        int iconCY = listY + ITEM_H / 2 - 1;
        uint8_t ir = item.kind == ItemKind::Hull ? 60  : 0;
        uint8_t ig = item.kind == ItemKind::Hull ? 120 : (sel ? 220 : 180);
        uint8_t ib = item.kind == ItemKind::Hull ? 220 : (sel ? 200 : 140);
        SDL_SetRenderDrawColor(r, ir, ig, ib, sel ? 230 : 150);
        drawShopIcon(r, item.id, item.kind, iconCX, iconCY, ICON_SZ);

        // Name
        SDL_Color nameCol;
        if      (maxed && item.kind != ItemKind::Hull) nameCol = {70, 110, 80, 180};
        else if (sel)                                   nameCol = {220, 240, 220, 255};
        else if (!afford && !maxed)                     nameCol = {160, 120, 80, 200};
        else                                            nameCol = {180, 200, 190, 210};

        std::string nameStr = item.name;
        if (hullActive) nameStr += " [EQ]";
        ctx.hud->drawLabel(nameStr, LIST_X + ICON_SZ + 12, listY + 4, nameCol);

        // Kind label sub-line
        SDL_Color kindCol = item.kind == ItemKind::Hull
            ? SDL_Color{60, 100, 160, 150} : SDL_Color{40, 120, 90, 150};
        std::string kindStr = item.kind == ItemKind::Hull ? "Hull Skin" : "Upgrade";
        if (item.maxPurchases > 1) {
            kindStr += "  " + std::to_string(owned) + "/" + std::to_string(item.maxPurchases);
        }
        ctx.hud->drawLabel(kindStr, LIST_X + ICON_SZ + 12, listY + 24, kindCol);

        // Cost — right-aligned
        std::string costStr;
        if (item.kind == ItemKind::Hull && owned > 0)    costStr = "OWNED";
        else if (maxed)                                   costStr = "MAXED";
        else                                              costStr = std::to_string(item.cost) + "CR";
        SDL_Color costCol = (maxed && item.kind != ItemKind::Hull)
            ? SDL_Color{50, 100, 60, 150}
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
    bool hullActive_sel = (sel.kind == ItemKind::Hull && ctx.saveData &&
                           ctx.saveData->activeHull == hullIdToActive(sel.id));
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
    uint8_t lr = sel.kind == ItemKind::Hull ? 60  : 0;
    uint8_t lg = sel.kind == ItemKind::Hull ? (uint8_t)(130 + (int)(40*pulse_sel)) : (uint8_t)(200 + (int)(20*pulse_sel));
    uint8_t lb = sel.kind == ItemKind::Hull ? 220 : (uint8_t)(170 + (int)(30*pulse_sel));
    SDL_SetRenderDrawColor(r, lr, lg, lb, 220);
    drawShopIcon(r, sel.id, sel.kind, iconCX, iconCY, LARGE_ICON);

    int detY = PANEL_Y + LARGE_ICON + 40;

    // Item name centred
    int nmw = ctx.hud->measureText(sel.name);
    ctx.hud->drawLabel(sel.name, DETAIL_X + DETAIL_W/2 - nmw/2, detY, {220, 240, 220, 255});
    detY += 28;

    // Category badge
    std::string badge;
    SDL_Color   badgeCol;
    if (sel.kind == ItemKind::Hull) {
        badge = "[ HULL SKIN ]"; badgeCol = {80, 140, 220, 200};
    } else if (std::string(sel.id) == "DEADBOLT") {
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
    if (sel.kind == ItemKind::Hull && owned_sel > 0) {
        costDisp    = hullActive_sel ? "// ACTIVE //" : "OWNED";
        costDispCol = {0, 220, 180, 255};
    } else if (maxed_sel) {
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

    // Buy / equip hint
    if (!maxed_sel || (sel.kind == ItemKind::Hull && owned_sel > 0)) {
        std::string hint;
        SDL_Color   hintCol;
        if (sel.kind == ItemKind::Hull && owned_sel > 0) {
            hint    = "[ ENTER ] to equip";
            hintCol = {0, 180, 140, 200};
        } else if (afford_sel) {
            hint    = "[ ENTER ] to purchase";
            hintCol = {0, 180, 140, 200};
        } else {
            hint    = "// INSUFFICIENT CREDITS //";
            hintCol = {180, 60, 40, 180};
        }
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
    std::string footer = "UP/DOWN: Navigate    ENTER: Buy/Equip    ESC/B: Back";
    int fw = ctx.hud->measureText(footer);
    ctx.hud->drawLabel(footer,
                       Constants::SCREEN_W / 2 - fw / 2,
                       Constants::SCREEN_H - 24,
                       {60, 100, 80, 180});

    SDL_RenderPresent(r);
}
