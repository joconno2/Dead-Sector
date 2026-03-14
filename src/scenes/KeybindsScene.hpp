#pragma once
#include "IScene.hpp"
#include "core/Bindings.hpp"

class KeybindsScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override;
    void onExit()                   override {}
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    Bindings m_bindings;         // local copy while editing
    int      m_cursor    = 0;    // 0..BINDING_COUNT-1 (row)
    int      m_col       = 0;    // 0 = keyboard column, 1 = controller column
    bool     m_listening = false; // waiting for input to rebind
    float    m_time      = 0.f;

    void save(SceneContext& ctx);
    void goBack(SceneContext& ctx);
};
