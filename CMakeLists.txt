cmake_minimum_required(VERSION 3.22)
project(racer C CXX)

set(CMAKE_CXX_STANDARD 17)

# Build raylib from vendored source
add_subdirectory(raylib)

# Game executable
add_executable(racer
  src/main.cpp
  src/game.cpp
  src/car.cpp
  src/player_car.cpp
  src/ai_car.cpp
  src/track.c
  src/effects.c
  src/hud.cpp
  src/audio.c
  src/first_person.cpp
)
target_link_libraries(racer PRIVATE raylib)
target_include_directories(racer PRIVATE src)