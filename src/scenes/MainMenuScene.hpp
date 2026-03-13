#pragma once
#include "IScene.hpp"

class MainMenuScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override;
    void onExit()                   override;
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    enum class MenuItem { NewRun = 0, Endless, Shop, Settings, Quit, COUNT };

    MenuItem m_cursor = MenuItem::NewRun;
    float    m_time   = 0.f;
    float    m_pulse  = 0.f;

    void selectCurrent(SceneContext& ctx);
    void startNewRun(SceneContext& ctx);
    void startEndless(SceneContext& ctx);
};
