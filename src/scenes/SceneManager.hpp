#pragma once
#include <memory>
#include <SDL.h>
#include "IScene.hpp"

struct SceneContext;

class SceneManager {
public:
    // Immediately activate the first scene (call from Game::init before first update)
    void start(std::unique_ptr<IScene> initial, SceneContext& ctx);

    // Request a transition; applied at the start of the next update()
    void replace(std::unique_ptr<IScene> next);

    void handleEvent(SDL_Event& ev, SceneContext& ctx);
    void update(float dt, SceneContext& ctx);
    void render(SceneContext& ctx);

private:
    std::unique_ptr<IScene> m_current;
    std::unique_ptr<IScene> m_pending;
};
