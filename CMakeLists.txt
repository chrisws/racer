cmake_minimum_required(VERSION 3.22)
project(racer C)

set(CMAKE_C_STANDARD 11)

# Build raylib from vendored source
add_subdirectory(raylib)

# Game executable
 add_executable(racer src/main.c src/game.c src/car.c src/track.c src/hud.c src/effects.c src/ai_car.c src/audio.c)
target_link_libraries(racer PRIVATE raylib)
target_include_directories(racer PRIVATE src)