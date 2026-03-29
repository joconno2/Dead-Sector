#include "Game.hpp"
#include "core/Constants.hpp"
#include "core/SaveSystem.hpp"
#include <fstream>
#include <iterator>
#include "core/DisplaySettings.hpp"
#include "renderer/VectorRenderer.hpp"
#include "renderer/HUD.hpp"
#include "scenes/MainMenuScene.hpp"
#include "world/NodeMap.hpp"
#include "systems/ModSystem.hpp"
#include "systems/ProgramSystem.hpp"
#include "audio/AudioSystem.hpp"
#include "steam/SteamManager.hpp"

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

Game::Game()  = default;
Game::~Game() = default;

bool Game::init(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0) {
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
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
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

    SDL_RenderSetLogicalSize(m_renderer, Constants::SCREEN_W, Constants::SCREEN_H);

    m_vrenderer = std::make_unique<VectorRenderer>(m_renderer);
    m_hud       = std::make_unique<HUD>(m_renderer, Constants::FONT_PATH, Constants::FONT_SIZE);
    m_scenes    = std::make_unique<SceneManager>();
    m_nodeMap   = std::make_unique<NodeMap>();
    m_mods      = std::make_unique<ModSystem>();
    m_programs  = std::make_unique<ProgramSystem>();
    m_audio     = std::make_unique<AudioSystem>();
    m_audio->init();

    m_steam = std::make_unique<SteamManager>();
    m_steam->init();  // no-op when STEAM_ENABLED is not defined

    // Build the shared context passed to all scenes
    m_ctx.window    = m_window;
    m_ctx.renderer  = m_renderer;
    m_ctx.vrenderer = m_vrenderer.get();
    m_ctx.hud       = m_hud.get();
    m_ctx.scenes    = m_scenes.get();
    m_ctx.nodeMap   = m_nodeMap.get();
    m_ctx.mods      = m_mods.get();
    m_ctx.programs  = m_programs.get();
    m_ctx.running   = &m_running;
    m_ctx.audio     = m_audio.get();
    m_ctx.steam     = m_steam.get();

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

    // Steam Cloud save: if a cloud save exists, pull it down and overwrite the local
    // file before loading. This ensures the freshest data wins on a new machine.
    {
        std::string cloudData;
        if (m_steam->cloudLoad(cloudData)) {
            std::ofstream f(SaveSystem::SAVE_PATH, std::ios::trunc);
            f << cloudData;
        }
    }

    // Load persistent save data
    m_saveData       = SaveSystem::load();
    m_ctx.saveData   = &m_saveData;

    // Apply saved volume settings
    if (m_audio) {
        m_audio->setMusicVolume(m_saveData.musicVolume * MIX_MAX_VOLUME / 100);
        m_audio->setSfxVolume(  m_saveData.sfxVolume   * MIX_MAX_VOLUME / 100);
    }

    // Command-line display overrides (do NOT write back to save — recovery only)
    // Usage: --windowed  --fullscreen  --borderless  --res 1280x720
    int  overrideMode = -1;   // -1 = no override
    int  overrideW    = m_saveData.windowedWidth;
    int  overrideH    = m_saveData.windowedHeight;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "--windowed")    overrideMode = DISP_WINDOWED;
        else if (arg == "--fullscreen")  overrideMode = DISP_FULLSCREEN;
        else if (arg == "--borderless")  overrideMode = DISP_BORDERLESS;
        else if (arg == "--res" && i + 1 < argc) {
            std::string res = argv[++i];
            auto sep = res.find('x');
            if (sep != std::string::npos) {
                try {
                    overrideW = std::stoi(res.substr(0, sep));
                    overrideH = std::stoi(res.substr(sep + 1));
                } catch (...) {}
            }
        }
    }
    int  applyMode = (overrideMode >= 0) ? overrideMode : m_saveData.displayMode;
    int  applyW    = overrideW;
    int  applyH    = overrideH;

    // Apply saved display mode on every launch (handles fullscreen/borderless/windowed)
    applyDisplaySettings(m_window, m_renderer, applyMode, applyW, applyH);

    m_scenes->start(std::make_unique<MainMenuScene>(), m_ctx);
    return true;
}

void Game::shutdown() {
    m_scenes.reset();

    // Push local save to Steam Cloud before shutdown
    if (m_steam && m_steam->isAvailable()) {
        std::ifstream f(SaveSystem::SAVE_PATH);
        std::string contents((std::istreambuf_iterator<char>(f)),
                              std::istreambuf_iterator<char>());
        if (!contents.empty())
            m_steam->cloudSave(contents);
    }

    if (m_steam) m_steam->shutdown();
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
            m_steam->tick();
            if (!m_steam->isOverlayActive())
                m_scenes->update(FIXED_DT, m_ctx);
            accumulator -= FIXED_TICKS;
        }

        m_scenes->render(m_ctx);
    }
}
