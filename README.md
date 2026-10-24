# Racer

![badge](.hali/badge.svg)

A 2D top-down racing game built with [raylib](https://www.raylib.com/). Race against AI cars on a circuit with chicanes and kinks.

## Features

- **Player car** with steering, acceleration, braking, and drift
- **3 AI cars** using waypoint-following pathfinding with speed adjustment in corners
- **Circuit track** with chicanes, kinks, and varied curves
- **HUD** with speed, lap counter, race/lap/best times, off-track warning, and a live minimap
- **Skid mark effects** when drifting at speed
- **Procedural audio** — engine pitch, AI car spatial audio, skid sounds, start beep
- **Game states** — menu → countdown (60s) → racing → finished
- **Camera** — follows player with lerp; F2/F3 zoom, F4 lock-on

## Controls

| Key | Action |
|-----|--------|
| W / Up | Accelerate |
| S / Down | Brake / Reverse |
| A / Left | Steer left |
| D / Right | Steer right |
| Space | Handbrake / Drift |
| F1 | Debug overlay |
| F2 / F3 | Zoom in / out |
| F4 | Lock camera to car |
| F5 | Crosshair |
| Esc | Back to menu |
| Enter | Start race / Return to menu |

## Architecture

```
src/
  main.cpp            Entry point, game loop, state dispatch
  game.h / game.cpp   Game struct, state update/draw functions
  car.h / car.cpp     Base Car class (position, angle, speed, Draw)
  player_car.h/.cpp   PlayerCar : Car — input, physics, drift
  ai_car.h / ai_car.cpp  AICar : Car — waypoint following, laps
  track.h / track.c   Track definition, on-track test, nearest point (C)
  effects.h/.c       Skid mark particles (C)
  hud.h / hud.cpp    Speed, laps, times, minimap
  audio.h / audio.c  Procedural engine/AI/skid audio (C)
```

### Class Hierarchy

```
Car (abstract)
├── PlayerCar   — steering, drift, keyboard input
└── AICar       — targetSpeed, waypointIndex, lap, track pointer
```

`Car` provides shared `Draw()` (body, windshield, 4 wheels) and `DrawRotatedRect()`.
Derived classes override `Update(float dt)`.

### C / C++ Interop

The track, effects, and audio modules are written in C. Their headers use
`extern "C"` guards so the C++ game code links against them correctly.
`CMakeLists.txt` enables both `C` and `CXX` languages.

## Building

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./racer
```

Requires: CMake ≥ 3.22, a C/C++17 compiler, X11 dev libraries (Linux).
raylib is built from the vendored source in `raylib/`.

## Issues Encountered

### `DrawRectanglePro` and `DrawTriangle` are broken

In the vendored raylib build, both `DrawRectanglePro` and `DrawTriangle`
silently fail to render anything. This was discovered during the C→C++
refactor when cars became invisible while the track (drawn with `DrawLineEx`)
rendered correctly.

**Workaround:** `Car::DrawRotatedRect()` fills a rotated rectangle using 8
parallel `DrawLineEx` calls instead of two `DrawTriangle` calls. Each line
spans the rectangle width at a proportional height, with thickness equal to
`height / 8`, producing a solid fill.

```cpp
// car.cpp — DrawRotatedRect fill
int N = 8;
float thick = h / N;
for (int i = 0; i < N; i++) {
  float t = (float)i / (float)N;
  Vector2 a = { c[0].x + (c[3].x - c[0].x) * t, ... };
  Vector2 b = { c[1].x + (c[2].x - c[1].x) * t, ... };
  DrawLineEx(a, b, thick, col);
}
```

### C/C++ linkage mismatch

When the C++ files (`game.cpp`, `ai_car.cpp`, `hud.cpp`) called functions
defined in C files (`track.c`, `effects.c`, `audio.c`), the linker reported
undefined references. C++ name-mangles the call sites but the C objects export
unmangled symbols.

**Fix:** Added `#ifdef __cplusplus extern "C" { #endif` guards to `track.h`,
`effects.h`, and `audio.h`.

### Virtual function signature mismatch

`AICar::Update(float dt, const Track *track)` did not match the base class
pure virtual `Car::Update(float dt)`, leaving `AICar` abstract and
uninstantiable.

**Fix:** Added a `const Track *track` member to `AICar`, changed the override
to `Update(float dt)`, and set the pointer in `AICar::InitAll()`.