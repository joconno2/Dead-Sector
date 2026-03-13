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

    // Hull: set as active
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

    ctx.hud->drawLabel("// WAREZ SHOP //", Constants::SCREEN_W / 2 - 100, 18, {0, 220, 180, 255});
    SDL_SetRenderDrawColor(r, 0, 180, 140, 80);
    SDL_RenderDrawLine(r, Constants::SCREEN_W / 2 - 140, 46,
                          Constants::SCREEN_W / 2 + 140, 46);

    if (ctx.saveData) {
        std::string credStr = "CREDITS: " + std::to_string(ctx.saveData->credits) + " CR";
        ctx.hud->drawLabel(credStr, Constants::SCREEN_W - 240, 18, {0, 220, 160, 255});

        std::string hullStr = "HULL: " + ctx.saveData->activeHull;
        ctx.hud->drawLabel(hullStr, 20, 18, {80, 160, 220, 200});
    }

    // Section labels
    ctx.hud->drawLabel("// PERMANENT UPGRADES", 30, 58, {60, 130, 100, 180});
    ctx.hud->drawLabel("// SHIP HULLS", 30, 58 + 8 * 50, {80, 120, 180, 180});

    constexpr int ITEM_H = 50;
    constexpr int LIST_X = Constants::SCREEN_W / 2 - 340;
    constexpr int LIST_W = 680;
    int           listY  = 76;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < CATALOG_SIZE; ++i) {
        if (i == 8) listY += 20; // gap between sections

        const ShopItem& item  = SHOP_CATALOG[i];
        bool            sel   = (i == m_cursor);
        float           pulse = sel ? (0.5f + 0.5f * std::sin(m_pulse)) : 0.f;
        int             owned = ctx.saveData ? ctx.saveData->purchaseCount(item.id) : 0;
        bool            maxed = (owned >= item.maxPurchases);
        bool            afford= ctx.saveData && ctx.saveData->credits >= item.cost;

        // Hull: check if it's currently equipped
        bool hullActive = (item.kind == ItemKind::Hull && ctx.saveData &&
                           ctx.saveData->activeHull == hullIdToActive(item.id));

        SDL_SetRenderDrawColor(r,
            item.kind == ItemKind::Hull ? 5 : 0,
            sel ? 40 : 20,
            item.kind == ItemKind::Hull ? (sel ? 55 : 30) : (sel ? 45 : 25),
            sel ? 210 : 130);
        SDL_Rect row = { LIST_X, listY, LIST_W, ITEM_H - 4 };
        SDL_RenderFillRect(r, &row);

        uint8_t bA = sel ? (uint8_t)(150 + 105*pulse) : 50;
        SDL_SetRenderDrawColor(r,
            item.kind == ItemKind::Hull ? bA/3 : bA/4,
            item.kind == ItemKind::Hull ? bA/2 : bA,
            item.kind == ItemKind::Hull ? bA : bA,
            sel ? 220 : 110);
        SDL_RenderDrawRect(r, &row);

        // Active hull indicator
        if (hullActive) {
            SDL_SetRenderDrawColor(r, 0, 220, 180, 60);
            SDL_Rect act = { LIST_X+2, listY+2, LIST_W-4, ITEM_H-8 };
            SDL_RenderFillRect(r, &act);
        }

        SDL_Color nameCol = maxed && item.kind != ItemKind::Hull
            ? SDL_Color{60, 100, 70, 180}
            : (afford ? SDL_Color{220, 220, 220, 255} : SDL_Color{160, 120, 80, 200});
        std::string nameStr = std::string(item.name);
        if (hullActive) nameStr += " [ACTIVE]";
        ctx.hud->drawLabel(nameStr,  LIST_X + 14, listY + 5,  nameCol);
        ctx.hud->drawLabel(item.desc, LIST_X + 14, listY + 25, {100, 160, 130, 200});

        // Cost / status
        SDL_Color costCol = (maxed && item.kind != ItemKind::Hull)
            ? SDL_Color{50, 100, 60, 160}
            : (afford ? SDL_Color{0, 220, 160, 255} : SDL_Color{220, 80, 50, 220});
        std::string costStr;
        if (item.kind == ItemKind::Hull && owned > 0)
            costStr = "OWNED";
        else if (maxed)
            costStr = "MAXED";
        else
            costStr = std::to_string(item.cost) + " CR";
        ctx.hud->drawLabel(costStr, LIST_X + LIST_W - 120, listY + 5, costCol);

        if (item.maxPurchases > 1) {
            std::string ownStr = "(" + std::to_string(owned) + "/" + std::to_string(item.maxPurchases) + ")";
            ctx.hud->drawLabel(ownStr, LIST_X + LIST_W - 120, listY + 25, {80, 120, 100, 180});
        }

        listY += ITEM_H;
    }

    if (m_statusTimer > 0.f) {
        float fade = std::min(1.f, m_statusTimer);
        SDL_Color sc = { 0, 220, 160, (uint8_t)(200 * fade) };
        int w = (int)m_statusMsg.size() * 12;
        ctx.hud->drawLabel(m_statusMsg,
                           Constants::SCREEN_W / 2 - w / 2,
                           Constants::SCREEN_H - 56, sc);
    }

    ctx.hud->drawLabel("UP/DOWN: Browse    ENTER: Buy/Equip    ESC/B: Back",
                       Constants::SCREEN_W / 2 - 250, Constants::SCREEN_H - 28,
                       {60, 100, 80, 180});

    SDL_RenderPresent(r);
}
