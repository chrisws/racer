#include "hud.h"
#include <stdio.h>

// Minimap settings
#define MINIMAP_X       1080
#define MINIMAP_Y       20
#define MINIMAP_W       180
#define MINIMAP_H       140
#define MINIMAP_SCALE   0.07f  // world-to-minimap scale

void HudDraw(const Game *game) {
  char buf[128];

  // --- Speed ---
  snprintf(buf, sizeof(buf), "Speed: %.0f", game->car.speed);
  DrawText(buf, 20, 50, 24, WHITE);

  // --- Lap info ---
  snprintf(buf, sizeof(buf), "Lap: %d / %d", game->lap + 1, game->totalLaps);
  DrawText(buf, 20, 90, 24, WHITE);

  // --- Times ---
  snprintf(buf, sizeof(buf), "Time:  %.2f", game->raceTime);
  DrawText(buf, 20, 130, 20, LIGHTGRAY);
  snprintf(buf, sizeof(buf), "Lap:   %.2f", game->lapTime);
  DrawText(buf, 20, 160, 20, LIGHTGRAY);
  if (game->bestLap > 0.0f) {
    snprintf(buf, sizeof(buf), "Best:  %.2f", game->bestLap);
    DrawText(buf, 20, 190, 20, GOLD);
  }

  // --- Off-track warning ---
  if (!TrackIsOnTrack(&game->track, game->car.position)) {
    DrawText("OFF TRACK!", 560, 30, 30, RED);
  }

  // --- Minimap ---
  DrawRectangle(MINIMAP_X, MINIMAP_Y, MINIMAP_W, MINIMAP_H, BLACK);
  DrawRectangleLines(MINIMAP_X, MINIMAP_Y, MINIMAP_W, MINIMAP_H, GRAY);

  // Draw track on minimap
  for (int i = 0; i < game->track.pointCount; i++) {
    int next = (i + 1) % game->track.pointCount;
    Vector2 a = game->track.points[i];
    Vector2 b = game->track.points[next];
    Vector2 ma = { MINIMAP_X + a.x * MINIMAP_SCALE, MINIMAP_Y + a.y * MINIMAP_SCALE };
    Vector2 mb = { MINIMAP_X + b.x * MINIMAP_SCALE, MINIMAP_Y + b.y * MINIMAP_SCALE };
    DrawLineEx(ma, mb, 2.0f, DARKGRAY);
  }

   // Draw AI car dots on minimap
  for (int i = 0; i < game->aiCarCount; i++) {
    Vector2 aiMM = {
      MINIMAP_X + game->aiCars[i].position.x * MINIMAP_SCALE,
      MINIMAP_Y + game->aiCars[i].position.y * MINIMAP_SCALE,
    };
    DrawCircleV(aiMM, 3.0f, game->aiCars[i].color);
  }

  // Draw player car dot on minimap
  Vector2 carMM = {
    MINIMAP_X + game->car.position.x * MINIMAP_SCALE,
    MINIMAP_Y + game->car.position.y * MINIMAP_SCALE,
  };
  DrawCircleV(carMM, 4.0f, RED);
}