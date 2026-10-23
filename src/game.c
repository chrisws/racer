  #include "game.h"
#include "hud.h"
#include "audio.h"
#include <stdio.h>
#include <math.h>

static int debugFrame = 0;

// --- Menu State ---

 void GameUpdateMenu(Game *game) {
  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
    game->state = GAME_RACING;
    game->lap = 0;
    game->lapTime = 0.0f;
    game->bestLap = 0.0f;
    game->raceTime = 0.0f;
    AudioPlayStartBeep();
  }
}

void GameDrawMenu(Game *game) {
  (void)game;
  DrawText("RACER", 540, 200, 80, WHITE);
  DrawText("Press ENTER to start", 500, 400, 20, LIGHTGRAY);
}

// --- Racing State ---

 void GameUpdateRacing(Game *game) {
  float dt = GetFrameTime();
  game->raceTime += dt;
  game->lapTime += dt;

  CarUpdate(&game->car, dt);

  // Off-track: apply extra drag
  if (!TrackIsOnTrack(&game->track, game->car.position)) {
    game->car.speed *= 0.95f;
  }

  // Skid marks when drifting
  if (game->car.drift > 0.3f && fabsf(game->car.speed) > 50.0f) {
    EffectsAddSkidMark(&game->effects, game->car.position);
    AudioPlaySkid();
  }

  // Update effects
  EffectsUpdate(&game->effects, dt);

  // Update AI cars
  for (int i = 0; i < game->aiCarCount; i++) {
    AICarUpdate(&game->aiCars[i], dt, &game->track);
  }

  // Engine sound (player)
  AudioUpdateEngine(game->car.speed, 400.0f);

  // AI car sounds: distance-based volume + Doppler pitch
  for (int i = 0; i < game->aiCarCount && i < 3; i++) {
    AICar *ai = &game->aiCars[i];
    float dx = game->car.position.x - ai->position.x;
    float dy = game->car.position.y - ai->position.y;
    float dist = sqrtf(dx * dx + dy * dy);
    // Relative speed: positive = approaching, negative = receding
    float aiVx = cosf(ai->angle) * ai->speed;
    float aiVy = sinf(ai->angle) * ai->speed;
    float invDist = (dist > 1.0f) ? 1.0f / dist : 0.0f;
    float relSpeed = (aiVx * dx + aiVy * dy) * invDist;
    AudioUpdateAICar(i, dist, relSpeed, ai->speed);
  }

   // Camera follow (smooth)
  float lerp = 5.0f * dt;
  game->camera.target.x += (game->car.position.x - game->camera.target.x) * lerp;
  game->camera.target.y += (game->car.position.y - game->camera.target.y) * lerp;

  // --- Zoom controls (persistent) ---
  if (IsKeyDown(KEY_F2)) {
    game->camera.zoom += 1.0f * dt;   // zoom in
    if (game->camera.zoom > 3.0f) game->camera.zoom = 3.0f;
  }
  if (IsKeyDown(KEY_F3)) {
    game->camera.zoom -= 1.0f * dt;   // zoom out
    if (game->camera.zoom < 0.3f) game->camera.zoom = 0.3f;
  }

  // --- Experimental: F4 hard-lock camera ---
  if (IsKeyDown(KEY_F4)) {
    game->camera.target = game->car.position;
    game->camera.zoom = 1.5f;
  }

  // Log every 30 frames
  debugFrame++;
  if (debugFrame % 30 == 0) {
    fprintf(stderr,
      "[DBG] car=(%.1f,%.1f) cam_tgt=(%.1f,%.1f) cam_off=(%.1f,%.1f) zoom=%.2f\n",
      game->car.position.x, game->car.position.y,
      game->camera.target.x, game->camera.target.y,
      game->camera.offset.x, game->camera.offset.y,
      game->camera.zoom);
  }

  // ESC to quit to menu
  if (IsKeyPressed(KEY_ESCAPE)) {
    game->state = GAME_MENU;
  }
}

  void GameDrawRacing(Game *game) {
  BeginMode2D(game->camera);
  DrawRectangleRec((Rectangle){ 0, 0, 2000, 2000 }, DARKGREEN);

  // Draw track
  TrackDraw(&game->track);

  // Draw skid marks
  EffectsDraw(&game->effects);

  // Draw AI cars
  for (int i = 0; i < game->aiCarCount; i++) {
    AICarDraw(&game->aiCars[i]);
  }

  // Draw player car (bigger, more detailed)
  float carW = 52.0f;
  float carH = 28.0f;
  float deg = game->car.angle * 180.0f / PI;

  // White outline for visibility
  Rectangle outline = { -carW / 2 - 2, -carH / 2 - 2, carW + 4, carH + 4 };
  DrawRectanglePro(outline, game->car.position, deg, WHITE);

  // Body
  Rectangle body = { -carW / 2, -carH / 2, carW, carH };
  DrawRectanglePro(body, game->car.position, deg, RED);

  // Windshield
  Rectangle wind = { carW * 0.1f, -carH * 0.3f, carW * 0.25f, carH * 0.6f };
  DrawRectanglePro(wind, game->car.position, deg, MAROON);

  // Wheels
  Color wheelColor = BLACK;
  float ww = 9.0f, wh = 5.0f;
  Rectangle wtl = { -carW * 0.3f, -carH * 0.55f, ww, wh };
  Rectangle wtr = { -carW * 0.3f, carH * 0.55f - wh, ww, wh };
  Rectangle wbl = { carW * 0.3f - ww, -carH * 0.55f, ww, wh };
  Rectangle wbr = { carW * 0.3f - ww, carH * 0.55f - wh, ww, wh };
  DrawRectanglePro(wtl, game->car.position, deg, wheelColor);
  DrawRectanglePro(wtr, game->car.position, deg, wheelColor);
  DrawRectanglePro(wbl, game->car.position, deg, wheelColor);
  DrawRectanglePro(wbr, game->car.position, deg, wheelColor);

   EndMode2D();

  // HUD overlay
  HudDraw(game);

  // F5: bright crosshair at car position (world space would be better, but screen-space works)
  if (IsKeyDown(KEY_F5)) {
    // Convert car world pos to screen pos for crosshair
    Vector2 screenPos = GetWorldToScreen2D(game->car.position, game->camera);
    DrawLine((int)screenPos.x - 30, (int)screenPos.y, (int)screenPos.x + 30, (int)screenPos.y, YELLOW);
    DrawLine((int)screenPos.x, (int)screenPos.y - 30, (int)screenPos.x, (int)screenPos.y + 30, YELLOW);
    DrawCircle((int)screenPos.x, (int)screenPos.y, 5, YELLOW);
  }

  // F1: debug info overlay
  if (IsKeyDown(KEY_F1)) {
    DrawText("=== DEBUG ===", 10, 10, 20, LIME);
    char buf[128];
    snprintf(buf, sizeof(buf), "car:    (%.1f, %.1f)", game->car.position.x, game->car.position.y);
    DrawText(buf, 10, 35, 20, LIME);
    snprintf(buf, sizeof(buf), "camTgt: (%.1f, %.1f)", game->camera.target.x, game->camera.target.y);
    DrawText(buf, 10, 60, 20, LIME);
    snprintf(buf, sizeof(buf), "camOff: (%.1f, %.1f)", game->camera.offset.x, game->camera.offset.y);
    DrawText(buf, 10, 85, 20, LIME);
    snprintf(buf, sizeof(buf), "zoom:   %.2f", game->camera.zoom);
    DrawText(buf, 10, 110, 20, LIME);
    snprintf(buf, sizeof(buf), "speed:  %.1f", game->car.speed);
    DrawText(buf, 10, 135, 20, LIME);
    snprintf(buf, sizeof(buf), "angle:  %.2f", game->car.angle);
    DrawText(buf, 10, 160, 20, LIME);
  }

  // Key hints
  DrawText("F2:zoom+ F3:zoom- F4:lockCam F5:crosshair", 10, 690, 16, GRAY);
}

// --- Finished State ---

void GameUpdateFinished(Game *game) {
  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
    game->state = GAME_MENU;
  }
}

void GameDrawFinished(Game *game) {
  DrawText("RACE COMPLETE!", 440, 200, 60, GOLD);
  char buf[128];
  snprintf(buf, sizeof(buf), "Total Time: %.2f", game->raceTime);
  DrawText(buf, 520, 320, 30, WHITE);
  snprintf(buf, sizeof(buf), "Best Lap: %.2f", game->bestLap);
  DrawText(buf, 520, 370, 30, WHITE);
  DrawText("Press ENTER for menu", 500, 500, 20, LIGHTGRAY);
}