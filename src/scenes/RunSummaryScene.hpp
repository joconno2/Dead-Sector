#pragma once
#include "IScene.hpp"
#include <string>
#include <vector>

class RunSummaryScene : public IScene {
public:
    RunSummaryScene(int score, int iceKilled, int nodesCleared, bool victory);

    void onEnter(SceneContext& ctx) override;
    void onExit()                   override;
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    int  m_score;
    int  m_iceKilled;
    int  m_nodesCleared;
    bool m_victory;

    float m_time     = 0.f;
    bool  m_canExit  = false;
};
