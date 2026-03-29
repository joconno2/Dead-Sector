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
class AudioSystem;
class SteamManager;

struct SceneContext {
    SDL_Window*         window     = nullptr;
    SDL_Renderer*       renderer   = nullptr;
    VectorRenderer*     vrenderer  = nullptr;
    HUD*                hud        = nullptr;
    SceneManager*       scenes     = nullptr;
    NodeMap*            nodeMap    = nullptr;
    ModSystem*          mods       = nullptr;
    ProgramSystem*      programs   = nullptr;
    SDL_GameController* controller = nullptr;
    bool*               running    = nullptr;
    AudioSystem*        audio      = nullptr;
    SteamManager*       steam      = nullptr;

    // Persistent save data
    SaveData*           saveData   = nullptr;

    // Per-run tracking
    int                 runCredits = 0;
    int                 runKills   = 0;
    int                 runNodes   = 0;

    // Multi-world run state
    int                 currentWorld = 0;  // 0=SECTOR ALPHA, 1=DEEP NET, 2=THE CORE

    // Difficulty (set in DifficultyScene, applied by CombatScene/TraceSystem)
    // 0=Runner (easy), 1=Decker (normal), 2=Netrunner (hard)
    int                 difficulty   = 1;

    // Endless mode
    bool                endlessMode = false;
    int                 endlessWave = 0;

    // Effects applied by Event nodes; consumed by the next CombatScene or MapScene jackIn
    float nextNodeStartTrace = 0.f;  // extra starting trace for next combat node
    int   bonusLives         = 0;    // extra lives granted for next combat node

    // Thematic credit bonuses accumulated during the run
    struct BonusEvent { std::string name; int amount; };
    std::vector<BonusEvent> runBonuses;
};
