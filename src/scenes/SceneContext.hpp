#pragma once
#include <SDL.h>

class VectorRenderer;
class HUD;
class SceneManager;
class NodeMap;
class ModSystem;
class ProgramSystem;

struct SceneContext {
    SDL_Renderer*      renderer   = nullptr;
    VectorRenderer*    vrenderer  = nullptr;
    HUD*               hud        = nullptr;
    SceneManager*      scenes     = nullptr;
    NodeMap*           nodeMap    = nullptr;
    ModSystem*         mods       = nullptr;
    ProgramSystem*     programs   = nullptr;
    SDL_GameController* controller = nullptr;
    bool*              running    = nullptr;
};
