#pragma once
#include "IScene.hpp"
#include "entities/Avatar.hpp"

class VictoryScene : public IScene {
public:
    explicit VictoryScene(int score, int iceKilled, int nodesCleared, HullType hull);

    void onEnter(SceneContext& ctx) override;
    void onExit()                   override {}
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    int      m_score;
    int      m_iceKilled;
    int      m_nodesCleared;
    HullType m_hull;

    float m_time       = 0.f;
    float m_canExit    = false;
    int   m_bonus      = 0;
    bool  m_isFinalWin = false;  // true only when currentWorld == 2 (THE CORE)

    void renderGoldenShip(SDL_Renderer* r, float cx, float cy, float scale) const;
};
