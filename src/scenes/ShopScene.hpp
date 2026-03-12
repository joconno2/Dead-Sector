#pragma once
#include "IScene.hpp"
#include <string>
#include <vector>

class ShopScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override;
    void onExit()                   override;
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    int   m_cursor = 0;
    float m_time   = 0.f;
    float m_pulse  = 0.f;

    std::string m_statusMsg;
    float       m_statusTimer = 0.f;

    void tryBuy(SceneContext& ctx);
};
