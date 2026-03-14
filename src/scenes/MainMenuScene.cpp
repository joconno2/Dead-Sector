#include "MainMenuScene.hpp"
#include "audio/AudioSystem.hpp"
#include "MapScene.hpp"
#include "CombatScene.hpp"
#include "ShopScene.hpp"
#include "ShipSelectScene.hpp"
#include "SettingsScene.hpp"
#include "CreditsScene.hpp"
#include "LeaderboardScene.hpp"
#include "WorldSelectScene.hpp"
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
    "HIGH SCORES",
    "SHOP",
    "SETTINGS",
    "CREDITS",
    "QUIT",
};
static constexpr int MENU_COUNT = 7;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MainMenuScene::spawnDriftObjs() {
    m_driftObjs.clear();
    std::mt19937 rng(42);  // fixed seed so background is deterministic
    std::uniform_real_distribution<float> px(0.f, (float)Constants::SCREEN_W);
    std::uniform_real_distribution<float> py(0.f, (float)Constants::SCREEN_H);
    std::uniform_real_distribution<float> spd(12.f, 28.f);
    std::uniform_real_distribution<float> ang(0.f, 6.283f);
    std::uniform_real_distribution<float> sc(0.5f, 1.6f);
    std::uniform_real_distribution<float> ph(0.f, 6.283f);
    std::uniform_int_distribution<int>   tp(0, 2);
    for (int i = 0; i < 10; ++i) {
        float a = ang(rng);
        float s = spd(rng);
        m_driftObjs.push_back({ px(rng), py(rng), std::cos(a)*s, std::sin(a)*s,
                                 ang(rng), (ang(rng) - 3.14f) * 0.15f,
                                 tp(rng), sc(rng), ph(rng) });
    }
}

void MainMenuScene::renderDriftObjs(SDL_Renderer* r) const {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (const auto& obj : m_driftObjs) {
        float pulse = 0.3f + 0.2f * std::sin(m_time * 0.8f + obj.phase);
        Uint8 alpha = (Uint8)(pulse * 60.f);

        SDL_SetRenderDrawColor(r, 0, 160, 220, alpha);

        float cx = obj.x, cy = obj.y;
        float s = obj.scale * 22.f;
        float a = obj.angle;

        // Build verts based on type
        std::vector<SDL_Point> pts;
        int sides = (obj.type == 0) ? 4 : (obj.type == 1) ? 3 : 6;
        float step = 6.283f / sides;
        float offset = (obj.type == 0) ? 0.785f : 0.f;  // diamond: 45° offset
        for (int i = 0; i <= sides; ++i) {
            float t = offset + a + i * step;
            pts.push_back({ (int)(cx + std::cos(t) * s), (int)(cy + std::sin(t) * s) });
        }
        SDL_RenderDrawLines(r, pts.data(), (int)pts.size());
    }
}

void MainMenuScene::onEnter(SceneContext& ctx) {
    m_time   = 0.f;
    m_pulse  = 0.f;
    m_cursor = MenuItem::NewRun;
    spawnDriftObjs();
    static constexpr const char* MENU_TRACK = "assets/music/Karl Casey - Jason Goes to Hell.mp3";
    if (ctx.audio) {
        // Restore full user volume (combat scene may have reduced it)
        if (ctx.saveData)
            ctx.audio->setMusicVolume(ctx.saveData->musicVolume * MIX_MAX_VOLUME / 100);
        // Only restart if not already streaming this track (prevents restart on sub-menu return)
        if (!ctx.audio->isPlaying(MENU_TRACK))
            ctx.audio->playMusicFrom(MENU_TRACK, 5.0, 1200);
    }
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
        if      (sc == SDL_SCANCODE_UP     || sc == SDL_SCANCODE_W) move(-1);
        else if (sc == SDL_SCANCODE_DOWN   || sc == SDL_SCANCODE_S) move(+1);
        else if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_SPACE) selectCurrent(ctx);
        else if (sc == SDL_SCANCODE_ESCAPE) { if (ctx.running) *ctx.running = false; }
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
        case MenuItem::Leaderboard:
            ctx.scenes->replace(std::make_unique<LeaderboardScene>());
            break;
        case MenuItem::Shop:
            ctx.scenes->replace(std::make_unique<ShopScene>());
            break;
        case MenuItem::Settings:
            ctx.scenes->replace(std::make_unique<SettingsScene>());
            break;
        case MenuItem::Credits:
            ctx.scenes->replace(std::make_unique<CreditsScene>());
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
    ctx.currentWorld = 0;
    ctx.endlessMode = false;
    ctx.endlessWave = 0;
    ctx.runBonuses.clear();

    applyShopUpgrades(ctx);

    // World selection first; WorldSelectScene → ShipSelectScene → MapScene → …
    ctx.scenes->replace(std::make_unique<WorldSelectScene>());
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

    // Ship selection first — ShipSelectScene(endless=true) launches CombatScene directly
    ctx.scenes->replace(std::make_unique<ShipSelectScene>(true));
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void MainMenuScene::update(float dt, SceneContext&) {
    m_time  += dt;
    m_pulse += dt * 2.5f;

    float W = (float)Constants::SCREEN_W;
    float H = (float)Constants::SCREEN_H;
    for (auto& obj : m_driftObjs) {
        obj.x     += obj.vx * dt;
        obj.y     += obj.vy * dt;
        obj.angle += obj.spin * dt;
        // Wrap at screen edges (with margin so shapes don't pop in)
        if (obj.x < -60.f) obj.x += W + 120.f;
        if (obj.x > W+60.f) obj.x -= W + 120.f;
        if (obj.y < -60.f) obj.y += H + 120.f;
        if (obj.y > H+60.f) obj.y -= H + 120.f;
    }
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void MainMenuScene::render(SceneContext& ctx) {
    VectorRenderer* vr = ctx.vrenderer;
    SDL_Renderer*   r  = ctx.renderer;

    vr->clear();
    vr->drawGrid(15, 20, 50);
    renderDriftObjs(r);

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
        float t   = std::fmod(m_time, CYCLE);
        int   idx = (int)(m_time / CYCLE) % QUOTE_COUNT;
        float alpha = (t < FADE) ? t / FADE : (t > CYCLE - FADE) ? (CYCLE - t) / FADE : 1.f;
        uint8_t qa = (uint8_t)(175 * alpha);
        uint8_t aa = (uint8_t)(120 * alpha);

        const Quote& q = QUOTE_LIST[idx];

        // Center each line using actual rendered width
        auto centredX = [&](const char* s) {
            return Constants::SCREEN_W / 2 - ctx.hud->measureText(s) / 2;
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

    // Version / build info — version bottom-left, nav hint centred
    ctx.hud->drawLabel(Constants::VERSION, 20, Constants::SCREEN_H - 28, {40, 70, 60, 140});
    {
        const char* hint = "ARROWS/DPAD: NAVIGATE   ENTER/A: SELECT";
        int hw = ctx.hud->measureText(hint);
        ctx.hud->drawLabel(hint, Constants::SCREEN_W / 2 - hw / 2,
                           Constants::SCREEN_H - 28, {40, 70, 60, 140});
    }

    SDL_RenderPresent(r);
}
