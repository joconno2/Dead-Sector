cmake_minimum_required(VERSION 3.16)
project(DeadSector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2     REQUIRED CONFIG)
find_package(SDL2_ttf REQUIRED CONFIG)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# WIN32 suppresses the console window on Windows
if(WIN32)
    add_executable(dead-sector WIN32 ${SOURCES})
else()
    add_executable(dead-sector ${SOURCES})
endif()

target_include_directories(dead-sector PRIVATE src)

if(WIN32)
    # Statically link SDL2 so the .exe is self-contained (no DLLs to ship).
    # SDL2main must come BEFORE SDL2-static so the linker can resolve its
    # SDL_* references from the static SDL2 archive.
    target_link_libraries(dead-sector PRIVATE
        SDL2::SDL2main
        SDL2::SDL2-static
        SDL2_ttf::SDL2_ttf-static
    )
    # Bake in libgcc and libstdc++ so no MinGW runtime DLLs are required.
    target_link_options(dead-sector PRIVATE -static-libgcc -static-libstdc++)
else()
    target_link_libraries(dead-sector PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
    )
endif()

# Copy assets directory next to the executable
add_custom_command(TARGET dead-sector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:dead-sector>/assets
)
