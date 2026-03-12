#pragma once
#include <SDL.h>
#include <string>
#include <vector>
#include "core/SaveSystem.hpp"

class VectorRenderer;
class HUD;
class SceneManager;
class NodeMap;
class ModSystem;
class ProgramSystem;

struct SceneContext {
    SDL_Renderer*       renderer   = nullptr;
    VectorRenderer*     vrenderer  = nullptr;
    HUD*                hud        = nullptr;
    SceneManager*       scenes     = nullptr;
    NodeMap*            nodeMap    = nullptr;
    ModSystem*          mods       = nullptr;
    ProgramSystem*      programs   = nullptr;
    SDL_GameController* controller = nullptr;
    bool*               running    = nullptr;

    // Persistent save data
    SaveData*           saveData   = nullptr;

    // Per-run tracking
    int                 runCredits = 0;
    int                 runKills   = 0;
    int                 runNodes   = 0;

    // Endless mode
    bool                endlessMode = false;
    int                 endlessWave = 0;

    // Debug
    bool                debugInvincible = false;

    // Thematic credit bonuses accumulated during the run
    struct BonusEvent { std::string name; int amount; };
    std::vector<BonusEvent> runBonuses;
};
