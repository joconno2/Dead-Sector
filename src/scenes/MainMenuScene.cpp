#include "MainMenuScene.hpp"
#include "MapScene.hpp"
#include "CombatScene.hpp"
#include "ShopScene.hpp"
#include "SceneContext.hpp"
#include "SceneManager.hpp"
#include "world/NodeMap.hpp"
#include "systems/ModSystem.hpp"
#include "systems/ProgramSystem.hpp"
#include "core/Constants.hpp"
#include "core/Programs.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "math/Vec2.hpp"

#include <SDL.h>
#include <cmath>
#include <random>
#include <string>

// ---------------------------------------------------------------------------
// Rotating cyberpunk quotes — cycle every 60s, ~12px per char at font size 20
// Max ~52 chars per line to stay centred on 1280px screen
// ---------------------------------------------------------------------------
struct Quote { const char* line1; const char* line2; const char* attr; };
static const Quote QUOTE_LIST[] = {
    { "\"The sky above the port was the color",
      "of television, tuned to a dead channel.\"",
      "Neuromancer  --  William Gibson, 1984" },
    { "\"The street finds its own uses for things.\"",
      nullptr,
      "Burning Chrome  --  William Gibson, 1982" },
    { "\"Cyberspace. A consensual hallucination.\"",
      nullptr,
      "Neuromancer  --  William Gibson, 1984" },
    { "\"The future is already here.",
      "It's just not evenly distributed.\"",
      "William Gibson" },
    { "\"I've seen things you people wouldn't believe.\"",
      nullptr,
      "Blade Runner  --  Ridley Scott, 1982" },
    { "\"Everything is deeply intertwingled.\"",
      nullptr,
      "Computer Lib  --  Ted Nelson, 1974" },
    { "\"The Net interprets censorship as damage",
      "and routes around it.\"",
      "John Gilmore, 1993" },
    { "\"A hacker enjoys playful cleverness.\"",
      nullptr,
      "Richard Stallman" },
    { "\"In cyberspace, the First Amendment",
      "is a local ordinance.\"",
      "John Perry Barlow, 1990" },
    { "\"Information wants to be free.\"",
      nullptr,
      "Stewart Brand, Hackers Conference, 1984" },
};
static constexpr int QUOTE_COUNT = (int)(sizeof(QUOTE_LIST) / sizeof(QUOTE_LIST[0]));

// ---------------------------------------------------------------------------
// Menu item labels
// ---------------------------------------------------------------------------
static const char* MENU_LABELS[] = {
    "NEW RUN",
    "ENDLESS",
    "SHOP",
    "QUIT",
};
static constexpr int MENU_COUNT = 4;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MainMenuScene::onEnter(SceneContext&) {
    m_time   = 0.f;
    m_pulse  = 0.f;
    m_cursor = MenuItem::NewRun;
}

void MainMenuScene::onExit() {}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void MainMenuScene::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    auto move = [&](int d) {
        int next = ((int)m_cursor + d + MENU_COUNT) % MENU_COUNT;
        m_cursor = (MenuItem)next;
    };

    if (ev.type == SDL_KEYDOWN) {
        const auto sc = ev.key.keysym.scancode;
        if      (sc == SDL_SCANCODE_UP   || sc == SDL_SCANCODE_W) move(-1);
        else if (sc == SDL_SCANCODE_DOWN || sc == SDL_SCANCODE_S) move(+1);
        else if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_SPACE) selectCurrent(ctx);
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        const auto btn = ev.cbutton.button;
        if      (btn == SDL_CONTROLLER_BUTTON_DPAD_UP)   move(-1);
        else if (btn == SDL_CONTROLLER_BUTTON_DPAD_DOWN) move(+1);
        else if (btn == SDL_CONTROLLER_BUTTON_A || btn == SDL_CONTROLLER_BUTTON_START)
            selectCurrent(ctx);
    }
}

void MainMenuScene::selectCurrent(SceneContext& ctx) {
    switch (m_cursor) {
        case MenuItem::NewRun:  startNewRun(ctx);  break;
        case MenuItem::Endless: startEndless(ctx); break;
        case MenuItem::Shop:
            ctx.scenes->replace(std::make_unique<ShopScene>());
            break;
        case MenuItem::Quit:
            if (ctx.running) *ctx.running = false;
            break;
        default: break;
    }
}

// Apply shop permanent upgrades to systems at run start
static void applyShopUpgrades(SceneContext& ctx) {
    if (!ctx.saveData) return;

    // EXTRA_PASSIVE: raise passive mod cap (+1 per stack, max 5)
    if (ctx.mods) {
        int cap = 3 + ctx.saveData->purchaseCount("EXTRA_PASSIVE");
        if (cap > 5) cap = 5;
        ctx.mods->setPassiveCap(cap);
    }

    // START_PROGRAM: grant one random program at run start
    if (ctx.programs && ctx.saveData->hasPurchase("START_PROGRAM") && ctx.programs->count() == 0) {
        const ProgramID pool[] = {
            ProgramID::FRAG, ProgramID::EMP, ProgramID::STEALTH,
            ProgramID::SHIELD, ProgramID::OVERDRIVE, ProgramID::FEEDBACK
        };
        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> d(0, 5);
        ctx.programs->add(pool[d(rng)]);
    }

    // REVEAL_MAP: all nodes visible from the start
    if (ctx.nodeMap && ctx.saveData->hasPurchase("REVEAL_MAP"))
        ctx.nodeMap->revealAll();
}

void MainMenuScene::startNewRun(SceneContext& ctx) {
    // Full reset
    if (ctx.mods)     ctx.mods->reset();
    if (ctx.programs) ctx.programs->reset();
    if (ctx.nodeMap)  ctx.nodeMap->reset();

    ctx.runCredits  = 0;
    ctx.runKills    = 0;
    ctx.runNodes    = 0;
    ctx.endlessMode = false;
    ctx.endlessWave = 0;
    ctx.runBonuses.clear();

    applyShopUpgrades(ctx);

    if (ctx.nodeMap)
        ctx.scenes->replace(std::make_unique<MapScene>(*ctx.nodeMap));
    else if (ctx.running)
        *ctx.running = false;
}

void MainMenuScene::startEndless(SceneContext& ctx) {
    // Full reset
    if (ctx.mods)     ctx.mods->reset();
    if (ctx.programs) ctx.programs->reset();

    ctx.runCredits  = 0;
    ctx.runKills    = 0;
    ctx.runNodes    = 0;
    ctx.endlessMode = true;
    ctx.endlessWave = 1;
    ctx.runBonuses.clear();

    applyShopUpgrades(ctx);

    NodeConfig cfg;
    cfg.nodeId         = -1;
    cfg.tier           = 1;
    cfg.objective      = NodeObjective::Sweep;
    cfg.sweepTarget    = 999999; // effectively never completes through normal objective
    cfg.traceTickRate  = 0.17f;  // very slow
    cfg.endless        = true;

    ctx.scenes->replace(std::make_unique<CombatScene>(cfg));
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void MainMenuScene::update(float dt, SceneContext&) {
    m_time  += dt;
    m_pulse += dt * 2.5f;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void MainMenuScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;

    vr->clear();
    vr->drawGrid(15, 20, 50);

    // Animated diagonal scan lines for atmosphere
    float scanY = std::fmod(m_time * 60.f, (float)Constants::SCREEN_H);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 150, 200, 12);
    SDL_RenderDrawLine(r, 0, (int)scanY, Constants::SCREEN_W, (int)(scanY + 40));

    vr->drawCRTOverlay();

    if (!ctx.hud) { SDL_RenderPresent(r); return; }

    // ---- TITLE ----
    float titlePulse = 0.6f + 0.4f * std::sin(m_pulse * 0.8f);
    SDL_Color titleCol = {
        (uint8_t)(0),
        (uint8_t)(200 + 55 * titlePulse),
        (uint8_t)(160 + 60 * titlePulse),
        255
    };

    int titleX = Constants::SCREEN_W / 2 - 120;
    int titleY = Constants::SCREEN_H / 4 - 30;
    ctx.hud->drawLabel("DEAD SECTOR", titleX, titleY, titleCol);

    // Glowing underline below title
    float glowA = 0.5f + 0.5f * std::sin(m_pulse);
    SDL_SetRenderDrawColor(r, 0, (uint8_t)(180 * glowA), (uint8_t)(220 * glowA), (uint8_t)(200 * glowA));
    SDL_RenderDrawLine(r, titleX - 10, titleY + 28, titleX + 230, titleY + 28);
    SDL_RenderDrawLine(r, titleX - 10, titleY + 29, titleX + 230, titleY + 29);

    // Rotating quote — 60s per quote, 1.5s crossfade, centered by char count
    {
        static constexpr float CYCLE = 60.f;
        static constexpr float FADE  = 1.5f;
        static constexpr int   CW    = 12; // approx pixels per char at font size 20
        float t   = std::fmod(m_time, CYCLE);
        int   idx = (int)(m_time / CYCLE) % QUOTE_COUNT;
        float alpha = (t < FADE) ? t / FADE : (t > CYCLE - FADE) ? (CYCLE - t) / FADE : 1.f;
        uint8_t qa = (uint8_t)(175 * alpha);
        uint8_t aa = (uint8_t)(120 * alpha);

        const Quote& q = QUOTE_LIST[idx];

        // Center each line independently
        auto centredX = [](const char* s) {
            return Constants::SCREEN_W / 2 - (int)(strlen(s) * CW / 2);
        };

        int attrY;
        ctx.hud->drawLabel(q.line1, centredX(q.line1), titleY + 36, {80, 175, 145, qa});
        if (q.line2) {
            ctx.hud->drawLabel(q.line2, centredX(q.line2), titleY + 56, {80, 175, 145, qa});
            attrY = titleY + 76;
        } else {
            attrY = titleY + 56;
        }
        ctx.hud->drawLabel(q.attr, centredX(q.attr), attrY, {55, 110, 90, aa});
    }

    // ---- MENU ITEMS ----
    int menuX = Constants::SCREEN_W / 2 - 80;
    int menuY = Constants::SCREEN_H / 2 - 20;
    int itemH = 42;

    for (int i = 0; i < MENU_COUNT; ++i) {
        bool sel = ((int)m_cursor == i);
        float p  = sel ? (0.5f + 0.5f * std::sin(m_pulse * 2.f)) : 0.f;

        SDL_Color col;
        if (sel) {
            col = { (uint8_t)(0),
                    (uint8_t)(200 + 55 * p),
                    (uint8_t)(160 + 80 * p),
                    255 };
        } else {
            col = { 60, 100, 80, 180 };
        }

        // Cursor indicator
        if (sel) {
            ctx.hud->drawLabel(">>", menuX - 30, menuY + i * itemH, col);
        }

        ctx.hud->drawLabel(MENU_LABELS[i], menuX, menuY + i * itemH, col);
    }

    // ---- Credits display ----
    if (ctx.saveData) {
        std::string credStr = "CREDITS: " + std::to_string(ctx.saveData->credits) + " CR";
        ctx.hud->drawLabel(credStr, 20, Constants::SCREEN_H - 50, {0, 180, 130, 200});

        std::string runStr = "RUNS: " + std::to_string(ctx.saveData->totalRuns)
                           + "  |  BEST: " + std::to_string(ctx.saveData->highScore);
        ctx.hud->drawLabel(runStr, 20, Constants::SCREEN_H - 28, {50, 100, 80, 160});
    }

    // Version / build info
    ctx.hud->drawLabel("v0.1  ARROW KEYS/DPAD: NAV  ENTER/A: SELECT",
                       Constants::SCREEN_W / 2 - 220,
                       Constants::SCREEN_H - 28,
                       {40, 70, 60, 140});

    SDL_RenderPresent(r);
}
