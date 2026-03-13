#pragma once
#include "IScene.hpp"
#include "core/Mods.hpp"
#include "core/Programs.hpp"
#include <vector>

// A single card offered to the player — either an active program or a mod
struct UpgradeCard {
    enum class Kind { Program, Mod };
    Kind      kind  = Kind::Mod;
    ProgramID progId = ProgramID::FRAG;
    ModID     modId  = ModID::KINETIC_CORE;
};

class NodeCompleteScene : public IScene {
public:
    // endless=true means nodeId==-1, return to a new CombatScene after upgrade pick
    NodeCompleteScene(int nodeId, int score, int iceKilled, int sweepTarget,
                      bool endless = false, int endlessWave = 0, float carryTrace = 0.f);

    void onEnter(SceneContext& ctx) override;
    void onExit()                   override;
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

private:
    enum class Phase { Result, UpgradePick };

    int   m_nodeId;
    int   m_score;
    int   m_iceKilled;
    int   m_sweepTarget;
    bool  m_endless     = false;
    int   m_endlessWave = 0;
    float m_carryTrace  = 0.f;
    float m_timer   = 0.f;
    bool  m_cleared = false;

    Phase                    m_phase      = Phase::Result;
    std::vector<UpgradeCard> m_offered;
    int                      m_cardCount = 3;   // EXTRA_OFFER raises this
    int                      m_cursor    = 0;
    float                    m_pulse     = 0.f;

    void buildOfferPool(SceneContext& ctx);
    void transitionToPick(SceneContext& ctx);
    void pickUpgrade(SceneContext& ctx);
    void returnToMap(SceneContext& ctx);

    void renderResult(SceneContext& ctx);
    void renderPick(SceneContext& ctx);
    void drawCard(SceneContext& ctx, int idx) const;
};
