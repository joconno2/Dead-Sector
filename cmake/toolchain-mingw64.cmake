# Cross-compilation toolchain for Windows x64 using MinGW-w64
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Statically link libgcc and libstdc++ so no MinGW runtime DLLs are needed
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")

# Point find_package at our bundled SDL2 Windows libs
set(SDL2_DIR       "${CMAKE_SOURCE_DIR}/deps/windows/SDL2-2.32.4/x86_64-w64-mingw32/lib/cmake/SDL2")
set(SDL2_ttf_DIR   "${CMAKE_SOURCE_DIR}/deps/windows/SDL2_ttf-2.24.0/x86_64-w64-mingw32/lib/cmake/SDL2_ttf")
set(SDL2_mixer_DIR "${CMAKE_SOURCE_DIR}/deps/windows/SDL2_mixer-2.8.1/x86_64-w64-mingw32/lib/cmake/SDL2_mixer")
