#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include "scenes/SceneContext.hpp"

// Debug overlay: toggle with LB+RB simultaneously on controller (or F12 on keyboard)
// Navigate with D-pad up/down, confirm with A / Enter
class DebugOverlay {
public:
    DebugOverlay() = default;

    bool isOpen() const { return m_open; }

    void handleEvent(SDL_Event& ev, SceneContext& ctx);
    void update(float dt, SceneContext& ctx);
    void render(SDL_Renderer* renderer, TTF_Font* font) const;

private:
    bool  m_open       = false;
    int   m_selected   = 0;
    float m_lbTimer    = 0.f;   // time LB has been held
    float m_rbTimer    = 0.f;   // time RB has been held

    static constexpr int NUM_ITEMS = 3;
    static constexpr const char* LABELS[NUM_ITEMS] = {
        "TOGGLE INVINCIBILITY",
        "ADD 10000 CREDITS",
        "CLEAR SAVE DATA",
    };

    void executeItem(int idx, SceneContext& ctx);
};
