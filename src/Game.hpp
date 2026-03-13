#pragma once
#include <SDL.h>
#include <memory>
#include "scenes/SceneContext.hpp"
#include "scenes/SceneManager.hpp"
#include "core/SaveSystem.hpp"
#include "debug/DebugOverlay.hpp"

class VectorRenderer;
class HUD;
class NodeMap;
class ModSystem;
class ProgramSystem;
class AudioSystem;

class Game {
public:
    Game();
    ~Game();

    bool init(int argc = 0, char* argv[] = nullptr);
    void run();
    void shutdown();

private:
    SDL_Window*         m_window     = nullptr;
    SDL_Renderer*       m_renderer   = nullptr;
    SDL_GameController* m_controller = nullptr;

    std::unique_ptr<VectorRenderer> m_vrenderer;
    std::unique_ptr<HUD>            m_hud;
    std::unique_ptr<SceneManager>   m_scenes;
    std::unique_ptr<NodeMap>        m_nodeMap;
    std::unique_ptr<ModSystem>      m_mods;
    std::unique_ptr<ProgramSystem>  m_programs;
    std::unique_ptr<AudioSystem>    m_audio;

    SaveData     m_saveData;
    SceneContext m_ctx;
    bool         m_running = false;
    DebugOverlay m_debug;
};
