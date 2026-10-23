# Top-Down Car Racing Game — Plan

**Goal:** Create a top-down 2D car racing game using vendored raylib with CMake build.

## Steps

| # | Step | Status |
|---|------|--------|
| 0 | Create root CMakeLists.txt that builds raylib as a subdirectory and compiles the game executable | ✅ |
| 1 | Implement core game loop, window setup, and game state machine (menu / racing / finished) | ✅ |
| 2 | Implement top-down car physics: acceleration, braking, steering, drift, and friction | ✅ |
| 3 | Create track system: track definition (waypoints/segments), rendering, and off-track boundary detection | ✅ |
| 4 | Implement HUD overlay: speedometer, lap counter, lap time, and a minimap | ✅ |
 | 5 | Add keyboard input handling, camera follow, and visual polish (particles, skid marks, screen shake) | ✅ |
| 6 | Add 3 AI opponent cars with waypoint-following behavior | ✅ |
| 7 | Add audio: engine hum (pitch varies with speed), start beep, skid noise | ✅ |
| 8 | Fix camera centering and improve car visibility (bigger, detailed rendering) | ✅ |

## Project Structure

```
racer/
├── CMakeLists.txt
├── plans/racing-game.md
├── raylib/          (vendored)
└── src/
    ├── main.c       — entry point, window, game loop
    ├── game.h       — Game struct, state enum
    ├── game.c       — state machine, update/draw dispatch
    ├── car.h        — Car struct
    ├── car.c        — physics (accel, brake, steer, drift, friction)
    ├── track.h      — Track struct
    ├── track.c      — waypoints, rendering, off-track detection
     ├── hud.h        — HUD interface
    ├── hud.c        — speed, lap, time, minimap
    ├── effects.h    — Effects struct
    ├── effects.c    — skid marks
    ├── ai_car.h     — AI car struct
    ├── ai_car.c     — waypoint-following AI opponents
    ├── audio.h      — Audio interface
    └── audio.c      — engine, beep, skid sounds
```

## Controls
- **W / ↑** — Accelerate
- **S / ↓** — Brake / Reverse
- **A / ←** — Steer left
- **D / →** — Steer right
- **ESC** — Back to menu
- **ENTER** — Start race / Return to menu

## Notes
- raylib is vendored at `raylib/` with full CMake support
- Game source lives in `src/` at project root
- Target: single executable `racer`
- Style: K&R, 2-space indents, explicit braces