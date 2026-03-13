cmake_minimum_required(VERSION 3.16)
project(DeadSector VERSION 0.2.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release if no build type specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

find_package(SDL2      REQUIRED CONFIG)
find_package(SDL2_ttf  REQUIRED CONFIG)

# SDL2_mixer: Windows uses the bundled MinGW cmake config; Linux uses the
# system library (only the versioned .so.0 may be present without -dev).
if(NOT WIN32)
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

# ---------------------------------------------------------------------------
# Sources & target
# ---------------------------------------------------------------------------

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# WIN32 suppresses the console window on Windows release builds
if(WIN32)
    add_executable(dead-sector WIN32 ${SOURCES})
else()
    add_executable(dead-sector ${SOURCES})
endif()

target_include_directories(dead-sector PRIVATE src deps/linux)

# Expose version string to source code
target_compile_definitions(dead-sector PRIVATE
    DEAD_SECTOR_VERSION="${PROJECT_VERSION}"
)

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
