#pragma once
#include <SDL.h>

// Display modes
enum { DISP_WINDOWED = 0, DISP_FULLSCREEN = 1, DISP_BORDERLESS = 2 };

// Common resolution presets (used for windowed + exclusive fullscreen)
struct Resolution { int w; int h; const char* label; };
static const Resolution RESOLUTIONS[] = {
    {  1280,  720, "1280x720   720p"    },
    {  1600,  900, "1600x900"           },
    {  1920, 1080, "1920x1080  1080p"   },
    {  2560, 1080, "2560x1080  UW1080"  },
    {  2560, 1440, "2560x1440  1440p"   },
    {  3440, 1440, "3440x1440  UW1440"  },
    {  3840, 2160, "3840x2160  4K"      },
};
static constexpr int RESOLUTION_COUNT = (int)(sizeof(RESOLUTIONS)/sizeof(RESOLUTIONS[0]));

// Returns index of RESOLUTIONS[] that best matches (w,h), or 0 if not found
inline int findResolutionIndex(int w, int h) {
    for (int i = 0; i < RESOLUTION_COUNT; ++i)
        if (RESOLUTIONS[i].w == w && RESOLUTIONS[i].h == h) return i;
    return 0;
}

// Apply display mode + (optionally) resolution to an SDL window+renderer.
// Always re-applies SDL_RenderSetLogicalSize(1280,720) so all game coords stay consistent.
// mode: DISP_WINDOWED=0, DISP_FULLSCREEN=1, DISP_BORDERLESS=2
// w,h: target size (used for windowed and exclusive fullscreen; ignored for borderless)
inline void applyDisplaySettings(SDL_Window* win, SDL_Renderer* ren, int mode, int w, int h) {
    if (!win || !ren) return;

    if (mode == DISP_BORDERLESS) {
        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else if (mode == DISP_FULLSCREEN) {
        // Exclusive fullscreen: try to match display mode, fall back gracefully
        SDL_SetWindowFullscreen(win, 0);
        SDL_DisplayMode target = {}, closest = {};
        target.w = w; target.h = h; target.refresh_rate = 0; target.format = 0;
        if (SDL_GetClosestDisplayMode(SDL_GetWindowDisplayIndex(win), &target, &closest)) {
            SDL_SetWindowDisplayMode(win, &closest);
        }
        SDL_SetWindowSize(win, w, h);
        SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN);
    } else { // Windowed
        SDL_SetWindowFullscreen(win, 0);
        SDL_SetWindowSize(win, w, h);
        SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    // Always maintain logical resolution — game logic never needs to know actual size
    SDL_RenderSetLogicalSize(ren, 1280, 720);
}
