#pragma once
#include "IScene.hpp"
#include "world/NodeMap.hpp"

class MapScene : public IScene {
public:
    explicit MapScene(NodeMap& nodeMap);

    void onEnter(SceneContext& ctx) override;
    void onExit()                   override;
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    NodeMap& m_nodeMap;
    int      m_cursor  = 0;    // currently selected node ID
    float    m_pulse   = 0.f;  // animation timer for selected room pulse

    void drawRoom(SDL_Renderer* r, const Node& n, bool selected, float pulse) const;
    void drawCorridors(SDL_Renderer* r) const;

    void moveCursor(int dCol, int dRow);
    bool jackIn(SceneContext& ctx);
};
