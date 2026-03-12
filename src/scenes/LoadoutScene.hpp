#pragma once
#include "IScene.hpp"
#include "CombatScene.hpp"
#include "core/Programs.hpp"
#include "core/Mods.hpp"
#include <array>
#include <vector>

class LoadoutScene : public IScene {
public:
    explicit LoadoutScene(NodeConfig cfg);

    void onEnter(SceneContext& ctx) override;
    void onExit()                   override;
    void handleEvent(SDL_Event& ev, SceneContext& ctx) override;
    void update(float dt, SceneContext& ctx)           override;
    void render(SceneContext& ctx)                     override;

    static constexpr int MOD_OFFERS = 3;  // public for static layout constants

private:
    enum class Phase { Programs, StartingUpgrade };

    static constexpr int POOL_SIZE  = 6;
    static constexpr int PICKS      = 3;

    NodeConfig                       m_cfg;
    std::array<ProgramID, POOL_SIZE> m_pool{};
    int                              m_cursor  = 0;
    std::vector<int>                 m_picks;
    float                            m_pulse   = 0.f;

    Phase                   m_phase      = Phase::Programs;
    std::vector<ModID>      m_modOffered;
    int                     m_modCursor  = 0;

    void buildPool();
    void moveCursor(int dCol, int dRow);
    void toggleSelect(SceneContext& ctx);
    void confirm(SceneContext& ctx);        // programs phase done
    void pickStartingMod(SceneContext& ctx);
    void launchCombat(SceneContext& ctx);

    void drawCard(SceneContext& ctx, int idx) const;
    void drawModCard(SceneContext& ctx, int idx) const;
    void renderUpgradePhase(SceneContext& ctx);
};
