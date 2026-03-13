#pragma once
#include "IScene.hpp"

class CreditsScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override;
    void onExit()                   override {}
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    float m_scroll = 0.f;   // current scroll position (pixels from top)
    float m_time   = 0.f;
    bool  m_done   = false;
};
