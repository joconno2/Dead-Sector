#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>

class HUD {
public:
    HUD(SDL_Renderer* renderer, const char* fontPath, int fontSize);
    ~HUD();

    bool isReady() const { return m_font != nullptr; }
    TTF_Font* font() const { return m_font; }

    void render(int score, float tracePercent,
                const class ProgramSystem* programs = nullptr,
                const class ModSystem*     mods     = nullptr,
                bool                       hasController = false);

    // Public text rendering for use by other scenes
    void drawLabel(const std::string& text, int x, int y, SDL_Color color);

    // Returns the rendered pixel width of a string (0 if font not ready)
    int measureText(const std::string& text) const;

    // Word-wraps text into successive lines within maxW pixels, returns lines drawn
    int drawWrapped(const std::string& text, int x, int y, int maxW,
                    SDL_Color color, int lineH = 20);

    // Boss health bar — call from CombatScene render when boss is active
    void drawBossBar(const char* name, int hp, int maxHp);

private:
    SDL_Renderer* m_renderer;
    TTF_Font*     m_font = nullptr;

    void drawText(const std::string& text, int x, int y, SDL_Color color);
    void drawTraceBar(float tracePercent);
    void drawProgramSlots(const ProgramSystem* programs, bool hasController);
};
