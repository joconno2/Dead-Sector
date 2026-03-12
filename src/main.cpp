#include "Game.hpp"
#include <SDL.h>
#include <SDL_ttf.h>
#include <cstdlib>
#include <iostream>

int main(int /*argc*/, char* /*argv*/[]) {
    Game game;
    if (!game.init()) {
        std::cerr << "Failed to initialize Dead Sector\n";
        return EXIT_FAILURE;
    }
    game.run();
    game.shutdown();
    return EXIT_SUCCESS;
}
