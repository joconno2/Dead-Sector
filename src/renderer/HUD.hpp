#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>

class HUD {
public:
    HUD(SDL_Renderer* renderer, const char* fontPath, int fontSize);
    ~HUD();

    // Returns false if font failed to load (HUD renders nothing)
    bool isReady() const { return m_font != nullptr; }

    void render(int score, float tracePercent,
                const class ProgramSystem* programs = nullptr,
                const class ModSystem*     mods     = nullptr);

    // Public text rendering for use by other scenes
    void drawLabel(const std::string& text, int x, int y, SDL_Color color);

private:
    SDL_Renderer* m_renderer;
    TTF_Font*     m_font = nullptr;

    void drawText(const std::string& text, int x, int y, SDL_Color color);
    void drawTraceBar(float tracePercent);
    void drawProgramSlots(const ProgramSystem* programs);
};
