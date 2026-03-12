#pragma once
#include <SDL.h>

struct SceneContext;

class IScene {
public:
    virtual ~IScene() = default;

    virtual void onEnter(SceneContext& ctx) = 0;
    virtual void onExit() = 0;
    virtual void handleEvent(SDL_Event& ev, SceneContext& ctx) = 0;
    virtual void update(float dt, SceneContext& ctx) = 0;
    virtual void render(SceneContext& ctx) = 0;
};
