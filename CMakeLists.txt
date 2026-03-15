cmake_minimum_required(VERSION 3.16)
project(DeadSector VERSION 0.3.10 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release if no build type specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Optional: Steamworks SDK integration
# Usage: cmake -DUSE_STEAM=ON ...
# Requires: Steamworks SDK extracted to deps/steamworks/
#   deps/steamworks/public/        — SDK headers
#   deps/steamworks/lib/linux64/   — libsteam_api.so   (Linux)
#   deps/steamworks/lib/win64/     — steam_api64.lib + steam_api64.dll (Windows)
# AppID: set in steam_appid.txt (480 = Spacewar test app; replace with your real AppID before ship)
# ---------------------------------------------------------------------------
option(USE_STEAM "Build with Steamworks SDK support" OFF)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

find_package(SDL2      REQUIRED CONFIG)
find_package(SDL2_ttf  REQUIRED CONFIG)

# SDL2_mixer: Windows/macOS use the cmake config package; Linux falls back to
# find_file because the system package may only ship the versioned .so.0.
if(WIN32 OR APPLE)
    find_package(SDL2_mixer REQUIRED CONFIG)
else()
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
endif()

# ---------------------------------------------------------------------------
# Sources & target
# ---------------------------------------------------------------------------

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# WIN32 suppresses the console window on Windows release builds.
# Include the .rc resource file so the executable gets the application icon.
# Drop assets/icon.ico (256/128/64/48/32/16 px) before building the Windows package.
if(WIN32)
    enable_language(RC)
    configure_file(src/platform/app.rc.in ${CMAKE_BINARY_DIR}/app.rc @ONLY)
    add_executable(dead-sector WIN32 ${SOURCES} ${CMAKE_BINARY_DIR}/app.rc)
else()
    add_executable(dead-sector ${SOURCES})
endif()

target_include_directories(dead-sector PRIVATE src deps/linux)

# Expose version string to source code
target_compile_definitions(dead-sector PRIVATE
    DEAD_SECTOR_VERSION="${PROJECT_VERSION}"
)

# ---------------------------------------------------------------------------
# Steam SDK (optional)
# ---------------------------------------------------------------------------
if(USE_STEAM)
    target_compile_definitions(dead-sector PRIVATE STEAM_ENABLED)
    target_include_directories(dead-sector PRIVATE
        ${CMAKE_SOURCE_DIR}/deps/steamworks/public
    )
    if(WIN32)
        target_link_libraries(dead-sector PRIVATE
            ${CMAKE_SOURCE_DIR}/deps/steamworks/lib/win64/steam_api64.lib
        )
        # Copy steam_api64.dll next to the executable
        add_custom_command(TARGET dead-sector POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/deps/steamworks/lib/win64/steam_api64.dll"
                $<TARGET_FILE_DIR:dead-sector>
            COMMENT "Copying steam_api64.dll"
        )
    elseif(APPLE)
        target_link_libraries(dead-sector PRIVATE
            ${CMAKE_SOURCE_DIR}/deps/steamworks/lib/osx/libsteam_api.dylib
        )
        add_custom_command(TARGET dead-sector POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/deps/steamworks/lib/osx/libsteam_api.dylib"
                $<TARGET_FILE_DIR:dead-sector>
            COMMENT "Copying libsteam_api.dylib"
        )
    else()
        target_link_libraries(dead-sector PRIVATE
            ${CMAKE_SOURCE_DIR}/deps/steamworks/lib/linux64/libsteam_api.so
        )
        add_custom_command(TARGET dead-sector POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/deps/steamworks/lib/linux64/libsteam_api.so"
                $<TARGET_FILE_DIR:dead-sector>
            COMMENT "Copying libsteam_api.so"
        )
    endif()
    message(STATUS "Steam integration: ON")
else()
    message(STATUS "Steam integration: OFF (use -DUSE_STEAM=ON to enable)")
endif()

# ---------------------------------------------------------------------------
# Linking
# ---------------------------------------------------------------------------

if(WIN32)
    # Statically link SDL2/SDL2_ttf so the .exe ships without those DLLs.
    # SDL2main must precede SDL2-static for correct symbol resolution.
    target_link_libraries(dead-sector PRIVATE
        SDL2::SDL2main
        SDL2::SDL2-static
        SDL2_ttf::SDL2_ttf-static
        SDL2_mixer::SDL2_mixer
    )
    # Bake in libgcc/libstdc++ — no MinGW runtime DLLs required.
    target_link_options(dead-sector PRIVATE -static-libgcc -static-libstdc++)
else()
    target_link_libraries(dead-sector PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
        SDL2_mixer::SDL2_mixer
    )
endif()

# ---------------------------------------------------------------------------
# Post-build: assets + Windows DLLs
# ---------------------------------------------------------------------------

# Copy assets next to the executable for every build
add_custom_command(TARGET dead-sector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:dead-sector>/assets
    COMMENT "Copying assets"
)

# ---------------------------------------------------------------------------
# Tests — not built for Windows cross-compile (SDL2 types still needed for
# headers, but no SDL_Init is called at runtime).
# Run with: cmake --build build-linux --target test  OR  ctest --test-dir build-linux
# ---------------------------------------------------------------------------
if(NOT WIN32)
    enable_testing()

    function(add_ds_test name)
        add_executable(${name} ${ARGN})
        target_include_directories(${name} PRIVATE src)
        target_link_libraries(${name} PRIVATE SDL2::SDL2 SDL2_ttf::SDL2_ttf SDL2_mixer::SDL2_mixer)
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    add_ds_test(test_save_system
        tests/test_save_system.cpp
        src/core/SaveSystem.cpp)

    add_ds_test(test_mod_system
        tests/test_mod_system.cpp
        src/systems/ModSystem.cpp)

    add_ds_test(test_trace_system
        tests/test_trace_system.cpp
        src/systems/TraceSystem.cpp)
endif()

# ---------------------------------------------------------------------------
# SDL2_mixer.dll links SDL2.dll dynamically at runtime even when the exe
# statically links SDL2, so both DLLs must ship with the Windows package.
if(WIN32)
    add_custom_command(TARGET dead-sector POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/deps/windows/SDL2-2.32.4/x86_64-w64-mingw32/bin/SDL2.dll"
            $<TARGET_FILE_DIR:dead-sector>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/deps/windows/SDL2_mixer-2.8.1/x86_64-w64-mingw32/bin/SDL2_mixer.dll"
            $<TARGET_FILE_DIR:dead-sector>
        COMMENT "Copying SDL2 runtime DLLs"
    )
endif()
