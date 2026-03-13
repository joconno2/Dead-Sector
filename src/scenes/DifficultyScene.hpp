#pragma once
#include "IScene.hpp"

// Shown after world selection, before ship selection.
// Player picks Runner / Decker / Netrunner difficulty.
class DifficultyScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override;
    void onExit()                   override {}
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    int   m_cursor = 1;  // default: Decker
    float m_pulse  = 0.f;
    float m_time   = 0.f;

    void confirm(SceneContext& ctx);
};
