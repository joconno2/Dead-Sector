cmake_minimum_required(VERSION 3.16)
project(DeadSector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2       REQUIRED CONFIG)
find_package(SDL2_ttf  REQUIRED CONFIG)

# SDL2_mixer: Windows uses the bundled MinGW cmake config; Linux uses the system library.
if(NOT WIN32)
    # Find the SDL2_mixer shared library. Without the -dev package only the
    # versioned .so.0 exists, so search for it explicitly.
    find_file(SDL2_MIXER_LIB
        NAMES libSDL2_mixer.so libSDL2_mixer-2.0.so libSDL2_mixer-2.0.so.0
        HINTS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib
        REQUIRED
    )
    add_library(SDL2_mixer::SDL2_mixer UNKNOWN IMPORTED)
    set_target_properties(SDL2_mixer::SDL2_mixer PROPERTIES
        IMPORTED_LOCATION "${SDL2_MIXER_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/linux"
    )
else()
    find_package(SDL2_mixer REQUIRED CONFIG)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# WIN32 suppresses the console window on Windows
if(WIN32)
    add_executable(dead-sector WIN32 ${SOURCES})
else()
    add_executable(dead-sector ${SOURCES})
endif()

target_include_directories(dead-sector PRIVATE src deps/linux)

if(WIN32)
    # Statically link SDL2 so the .exe is self-contained (no DLLs to ship).
    # SDL2main must come BEFORE SDL2-static so the linker can resolve its
    # SDL_* references from the static SDL2 archive.
    target_link_libraries(dead-sector PRIVATE
        SDL2::SDL2main
        SDL2::SDL2-static
        SDL2_ttf::SDL2_ttf-static
        SDL2_mixer::SDL2_mixer
    )
    # Bake in libgcc and libstdc++ so no MinGW runtime DLLs are required.
    target_link_options(dead-sector PRIVATE -static-libgcc -static-libstdc++)
else()
    target_link_libraries(dead-sector PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
        SDL2_mixer::SDL2_mixer
    )
endif()

# Copy assets directory next to the executable
add_custom_command(TARGET dead-sector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:dead-sector>/assets
)

# On Windows, SDL2_mixer ships as a DLL — copy it next to the exe
if(WIN32)
    add_custom_command(TARGET dead-sector POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/deps/windows/SDL2_mixer-2.8.1/x86_64-w64-mingw32/bin/SDL2_mixer.dll"
            $<TARGET_FILE_DIR:dead-sector>
    )
endif()
