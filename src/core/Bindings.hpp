#pragma once
#include <SDL.h>

// All rebindable actions — one keyboard scancode and one controller button each.
// Arrow keys / DPAD / right-trigger are always non-rebindable secondary fallbacks.
struct Bindings {
    // Keyboard (primary)
    SDL_Scancode thrust   = SDL_SCANCODE_W;
    SDL_Scancode rotLeft  = SDL_SCANCODE_A;
    SDL_Scancode rotRight = SDL_SCANCODE_D;
    SDL_Scancode fire     = SDL_SCANCODE_LSHIFT;
    SDL_Scancode prog0    = SDL_SCANCODE_Q;
    SDL_Scancode prog1    = SDL_SCANCODE_E;
    SDL_Scancode prog2    = SDL_SCANCODE_R;

    // Controller buttons (primary)
    SDL_GameControllerButton ctlThrust   = SDL_CONTROLLER_BUTTON_A;
    SDL_GameControllerButton ctlRotLeft  = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    SDL_GameControllerButton ctlRotRight = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    SDL_GameControllerButton ctlFire     = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    SDL_GameControllerButton ctlProg0    = SDL_CONTROLLER_BUTTON_X;
    SDL_GameControllerButton ctlProg1    = SDL_CONTROLLER_BUTTON_Y;
    SDL_GameControllerButton ctlProg2    = SDL_CONTROLLER_BUTTON_B;
};

struct BindingDef {
    const char*              label;
    SDL_Scancode             Bindings::* kbField;   // keyboard binding
    SDL_GameControllerButton Bindings::* ctlField;  // controller binding
};

inline constexpr int BINDING_COUNT = 7;

inline const BindingDef BINDING_DEFS[BINDING_COUNT] = {
    { "THRUST",    &Bindings::thrust,   &Bindings::ctlThrust   },
    { "ROTATE L",  &Bindings::rotLeft,  &Bindings::ctlRotLeft  },
    { "ROTATE R",  &Bindings::rotRight, &Bindings::ctlRotRight },
    { "FIRE",      &Bindings::fire,     &Bindings::ctlFire     },
    { "PROGRAM 1", &Bindings::prog0,    &Bindings::ctlProg0    },
    { "PROGRAM 2", &Bindings::prog1,    &Bindings::ctlProg1    },
    { "PROGRAM 3", &Bindings::prog2,    &Bindings::ctlProg2    },
};

// Human-readable controller button name (SDL's own names are lowercase/verbose)
inline const char* ctlButtonName(SDL_GameControllerButton b) {
    switch (b) {
        case SDL_CONTROLLER_BUTTON_A:             return "A";
        case SDL_CONTROLLER_BUTTON_B:             return "B";
        case SDL_CONTROLLER_BUTTON_X:             return "X";
        case SDL_CONTROLLER_BUTTON_Y:             return "Y";
        case SDL_CONTROLLER_BUTTON_BACK:          return "SELECT";
        case SDL_CONTROLLER_BUTTON_GUIDE:         return "GUIDE";
        case SDL_CONTROLLER_BUTTON_START:         return "START";
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return "L.STICK";
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return "R.STICK";
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return "L.BUMP";
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "R.BUMP";
        case SDL_CONTROLLER_BUTTON_DPAD_UP:       return "DPAD UP";
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return "DPAD DN";
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return "DPAD LT";
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return "DPAD RT";
        default:                                  return "???";
    }
}
