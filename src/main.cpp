#include "Game.hpp"
#include <SDL.h>
#include <SDL_ttf.h>
#include <cstdlib>
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    // On Windows WIN32 subsystem, stderr goes nowhere — redirect to a log file
#ifdef _WIN32
    static std::ofstream logFile("dead-sector.log", std::ios::trunc);
    if (logFile.is_open()) std::cerr.rdbuf(logFile.rdbuf());
#endif

    Game game;
    if (!game.init(argc, argv)) {
        std::cerr << "Failed to initialize Dead Sector\n";
        return EXIT_FAILURE;
    }
    game.run();
    game.shutdown();
    return EXIT_SUCCESS;
}
