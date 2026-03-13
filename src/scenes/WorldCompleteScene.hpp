#pragma once
#include "IScene.hpp"

class WorldCompleteScene : public IScene {
public:
    explicit WorldCompleteScene(int worldId, int score, int iceKilled);

    void onEnter(SceneContext& ctx) override;
    void onExit()                   override {}
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    int   m_worldId;
    int   m_score;
    int   m_iceKilled;

    float m_time     = 0.f;
    bool  m_canExit  = false;

    void advance(SceneContext& ctx);
};
