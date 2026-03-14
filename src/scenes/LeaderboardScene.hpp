#pragma once
#include "IScene.hpp"

class LeaderboardScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override;
    void onExit()                   override {}
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    float m_time     = 0.f;
    bool  m_canExit  = false;
};
