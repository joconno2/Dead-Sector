#pragma once
#include "IScene.hpp"
#include <vector>

class MainMenuScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override;
    void onExit()                   override;
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    enum class MenuItem { NewRun = 0, Endless, Leaderboard, Shop, Settings, Credits, Quit, COUNT };

    MenuItem m_cursor = MenuItem::NewRun;
    float    m_time   = 0.f;
    float    m_pulse  = 0.f;

    // Background drifting ICE silhouettes
    struct DriftObj {
        float x, y;    // position (pixels)
        float vx, vy;  // velocity (px/s)
        float angle;   // current rotation (radians)
        float spin;    // rotation speed (rad/s)
        int   type;    // 0=diamond, 1=triangle, 2=hex
        float scale;   // size multiplier
        float phase;   // individual time phase for opacity pulse
    };
    std::vector<DriftObj> m_driftObjs;

    void spawnDriftObjs();
    void renderDriftObjs(SDL_Renderer* r) const;

    void selectCurrent(SceneContext& ctx);
    void startNewRun(SceneContext& ctx);
    void startEndless(SceneContext& ctx);
};
