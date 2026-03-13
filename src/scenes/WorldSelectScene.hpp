#pragma once
#include "IScene.hpp"

class WorldSelectScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override;
    void onExit()                   override {}
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    int   m_cursor      = 0;
    float m_time        = 0.f;
    float m_pulse       = 0.f;
    int   m_unlocked    = 1;    // populated in onEnter from saveData

    void confirm(SceneContext& ctx);

    // Vector art drawers — cx,cy is the art area center
    void drawCircuitArt (SDL_Renderer* r, float cx, float cy, float t) const;
    void drawNeuralArt  (SDL_Renderer* r, float cx, float cy, float t) const;
    void drawReactorArt (SDL_Renderer* r, float cx, float cy, float t) const;
};
