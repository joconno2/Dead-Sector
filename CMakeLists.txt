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

# SDL2main must come BEFORE SDL2 so the static SDL2main.a can resolve
# its SDL_* references from the SDL2 import library.
if(WIN32)
    target_link_libraries(dead-sector PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(dead-sector PRIVATE
    SDL2::SDL2
    SDL2_ttf::SDL2_ttf
)

# Copy assets directory next to the executable
add_custom_command(TARGET dead-sector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:dead-sector>/assets
)

# Windows: copy SDL2 DLLs next to the executable
if(WIN32)
    set(SDL2_DLL_DIR     "${CMAKE_SOURCE_DIR}/deps/windows/SDL2-2.32.4/x86_64-w64-mingw32/bin")
    set(SDL2_TTF_DLL_DIR "${CMAKE_SOURCE_DIR}/deps/windows/SDL2_ttf-2.24.0/x86_64-w64-mingw32/bin")

    add_custom_command(TARGET dead-sector POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_DLL_DIR}/SDL2.dll"
            $<TARGET_FILE_DIR:dead-sector>/SDL2.dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_TTF_DLL_DIR}/SDL2_ttf.dll"
            $<TARGET_FILE_DIR:dead-sector>/SDL2_ttf.dll
    )
endif()
