cmake_minimum_required(VERSION 3.16)
project(DeadSector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2     REQUIRED CONFIG)
find_package(SDL2_ttf REQUIRED CONFIG)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(dead-sector ${SOURCES})

target_include_directories(dead-sector PRIVATE src)

target_link_libraries(dead-sector PRIVATE
    SDL2::SDL2
    SDL2_ttf::SDL2_ttf
)

add_custom_command(TARGET dead-sector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:dead-sector>/assets
)
