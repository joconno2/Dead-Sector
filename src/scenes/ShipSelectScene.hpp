#pragma once
#include "IScene.hpp"
#include "entities/Avatar.hpp"
#include "core/SaveSystem.hpp"

class ShipSelectScene : public IScene {
public:
    explicit ShipSelectScene(bool endless = false) : m_endless(endless) {}

    void onEnter(SceneContext& ctx) override;
    void onExit()                   override {}
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    static constexpr int NUM_HULLS = 5;
    static constexpr HullType HULL_ORDER[NUM_HULLS] = {
        HullType::Delta, HullType::Raptor, HullType::Mantis,
        HullType::Battle, HullType::Blade
    };

    bool  m_endless = false;
    int   m_cursor  = 0;
    float m_time    = 0.f;
    float m_pulse   = 0.f;
    bool  m_canExit = true;

    bool isUnlocked(int idx, const SaveData& save) const;
    void confirm(SceneContext& ctx);
    void renderShipSilhouette(SDL_Renderer* r, const HullStats& stats, HullType hull,
                               float cx, float cy, float scale,
                               bool golden, bool locked, float time) const;
};
