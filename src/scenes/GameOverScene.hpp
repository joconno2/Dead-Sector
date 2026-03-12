#pragma once
#include "IScene.hpp"

class GameOverScene : public IScene {
public:
    explicit GameOverScene(int finalScore) : m_score(finalScore) {}

    void onEnter(SceneContext& ctx) override;
    void onExit() override;
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx) override;
    void render(SceneContext& ctx) override;

private:
    int   m_score = 0;
    float m_time  = 0.f;   // seconds since GameOver (for pulsing effects)
};
