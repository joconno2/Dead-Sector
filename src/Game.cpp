#include "Game.hpp"
#include "core/Constants.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "scenes/MapScene.hpp"
#include "world/NodeMap.hpp"
#include "systems/ModSystem.hpp"
#include "systems/ProgramSystem.hpp"

#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

Game::Game()  = default;
Game::~Game() = default;

bool Game::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << "\n";
        return false;
    }
    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init error: " << TTF_GetError() << "\n";
        return false;
    }

    m_window = SDL_CreateWindow(
        "DEAD SECTOR",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        Constants::SCREEN_W, Constants::SCREEN_H,
        SDL_WINDOW_SHOWN
    );
    if (!m_window) {
        std::cerr << "SDL_CreateWindow error: " << SDL_GetError() << "\n";
        return false;
    }

    m_renderer = SDL_CreateRenderer(
        m_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!m_renderer) {
        std::cerr << "SDL_CreateRenderer error: " << SDL_GetError() << "\n";
        return false;
    }

    m_vrenderer = std::make_unique<VectorRenderer>(m_renderer);
    m_hud       = std::make_unique<HUD>(m_renderer, Constants::FONT_PATH, Constants::FONT_SIZE);
    m_scenes    = std::make_unique<SceneManager>();
    m_nodeMap   = std::make_unique<NodeMap>();
    m_mods      = std::make_unique<ModSystem>();
    m_programs  = std::make_unique<ProgramSystem>();

    // Build the shared context passed to all scenes
    m_ctx.renderer  = m_renderer;
    m_ctx.vrenderer = m_vrenderer.get();
    m_ctx.hud       = m_hud.get();
    m_ctx.scenes    = m_scenes.get();
    m_ctx.nodeMap   = m_nodeMap.get();
    m_ctx.mods      = m_mods.get();
    m_ctx.programs  = m_programs.get();
    m_ctx.running   = &m_running;

    // Open first available game controller
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            m_controller = SDL_GameControllerOpen(i);
            if (m_controller) {
                std::cout << "Controller: " << SDL_GameControllerName(m_controller) << "\n";
                break;
            }
        }
    }
    m_ctx.controller = m_controller;

    m_scenes->start(std::make_unique<MapScene>(*m_nodeMap), m_ctx);
    return true;
}

void Game::shutdown() {
    m_scenes.reset();
    m_vrenderer.reset();
    m_hud.reset();
    m_nodeMap.reset();

    if (m_controller) { SDL_GameControllerClose(m_controller); m_controller = nullptr; }
    if (m_renderer)   { SDL_DestroyRenderer(m_renderer);      m_renderer   = nullptr; }
    if (m_window)     { SDL_DestroyWindow(m_window);           m_window     = nullptr; }
    TTF_Quit();
    SDL_Quit();
}

// ---------------------------------------------------------------------------
// Game loop
// ---------------------------------------------------------------------------

void Game::run() {
    constexpr float  FIXED_DT    = 1.f / 60.f;
    constexpr Uint64 FIXED_TICKS = static_cast<Uint64>(FIXED_DT * 1000.0);

    Uint64 accumulator = 0;
    Uint64 prevTime    = SDL_GetTicks64();

    m_running = true;
    while (m_running) {
        Uint64 now     = SDL_GetTicks64();
        Uint64 elapsed = now - prevTime;
        prevTime       = now;
        if (elapsed > 250) elapsed = 250; // spiral-of-death guard

        // Process SDL events at Game level (quit) + pass to active scene
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) m_running = false;
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) m_running = false;

            // Controller hot-plug
            if (ev.type == SDL_CONTROLLERDEVICEADDED && !m_controller) {
                m_controller = SDL_GameControllerOpen(ev.cdevice.which);
                m_ctx.controller = m_controller;
                if (m_controller)
                    std::cout << "Controller connected: "
                              << SDL_GameControllerName(m_controller) << "\n";
            }
            if (ev.type == SDL_CONTROLLERDEVICEREMOVED && m_controller) {
                SDL_GameControllerClose(m_controller);
                m_controller     = nullptr;
                m_ctx.controller = nullptr;
                std::cout << "Controller disconnected.\n";
            }

            m_scenes->handleEvent(ev, m_ctx);
        }

        accumulator += elapsed;
        while (accumulator >= FIXED_TICKS) {
            m_scenes->update(FIXED_DT, m_ctx);
            accumulator -= FIXED_TICKS;
        }

        m_scenes->render(m_ctx);
    }
}
