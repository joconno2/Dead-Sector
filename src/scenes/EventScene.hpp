#pragma once
#include "IScene.hpp"
#include "core/Events.hpp"

// Shown when the player jacks into an Event node.
// Picks a random event, displays flavor text, applies the effect,
// marks the node cleared, and returns to MapScene on any key/button.
class EventScene : public IScene {
public:
    explicit EventScene(int nodeId, unsigned int seed);

    void onEnter(SceneContext& ctx) override;
    void onExit()                   override {}
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    int          m_nodeId;
    unsigned int m_seed;
    const EventDef* m_event = nullptr;

    float m_timer       = 0.f;   // time since entry (used for animation)
    bool  m_applied     = false; // effect applied exactly once in onEnter
    bool  m_canConfirm  = false; // prevent immediate dismissal on key-held-down
};
