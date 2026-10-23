#include "game.h"
#include "hud.h"
#include "audio.h"
#include "first_person.h"
#include <stdio.h>
#include <math.h>

static int debugFrame = 0;

void GameUpdateMenu(Game *game) {
  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
    game->state = GAME_COUNTDOWN;
    game->lap = 0;
    game->lapTime = 0.0f;
    game->bestLap = 0.0f;
    game->raceTime = 0.0f;
    game->countdownTimer = COUNTDOWN_DURATION;
    game->car = PlayerCar((Vector2){400.0f, 900.0f}, 0.0f);
    AICar::InitAll(game->aiCars, game->aiCarCount, &game->track);
    game->camera.target = (Vector2){450.0f, 900.0f};
    game->camera.zoom = 2.0f;
    AudioPlayStartBeep();
  }
}

void GameDrawMenu(Game *game) {
  (void)game;
  DrawText("RACER", 540, 200, 80, WHITE);
  DrawText("Press ENTER to start", 500, 400, 20, LIGHTGRAY);
}

static int lastCountdownSec = -1;

void GameUpdateCountdown(Game *game) {
  float dt = GetFrameTime();
  game->countdownTimer -= dt;
  game->car.speed = 0.0f;
  for (int i = 0; i < game->aiCarCount; i++) {
    game->aiCars[i].speed = 0.0f;
  }
  game->camera.target = (Vector2){450.0f, 900.0f};
  game->camera.zoom = 2.0f;

  int sec = (int)ceilf(game->countdownTimer);
  if (sec != lastCountdownSec) {
    lastCountdownSec = sec;
    if (sec >= 0) {
      AudioPlayCountdownBeep(sec);
    }
  }

  if (IsKeyPressed(KEY_ESCAPE)) {
    game->state = GAME_MENU;
    lastCountdownSec = -1;
  }
  if (game->countdownTimer <= 0.0f) {
    game->countdownTimer = 0.0f;
    game->state = GAME_RACING;
    lastCountdownSec = -1;
  }
}

void GameDrawCountdown(Game *game) {
  if (game->viewMode == VIEW_FIRST_PERSON) {
    FirstPersonDraw(game);
  } else {
    BeginMode2D(game->camera);
    DrawRectangleRec((Rectangle){0, 0, 2000, 2000}, DARKGREEN);
    TrackDraw(&game->track);
    for (int i = 0; i < game->aiCarCount; i++) {
      game->aiCars[i].Draw();
    }
    game->car.Draw();
    EndMode2D();
  }

  int remaining = (int)ceilf(game->countdownTimer);

  // F1 start lights: 5 circles, light up during last 5 seconds
  int lightY = 160;
  int lightR = 28;
  int lightGap = 70;
  int startX = 640 - 2 * lightGap;

  for (int i = 0; i < 5; i++) {
    int cx = startX + i * lightGap;
    bool lit = (remaining <= (5 - i)) && (remaining > 0);
    Color col = lit ? RED : (Color){60, 60, 60, 255};
    DrawCircle(cx, lightY, lightR, col);
    DrawCircleLines(cx, lightY, lightR, (Color){100, 100, 100, 255});
    if (lit) {
      // Glow effect
      DrawCircle(cx, lightY, lightR - 8, (Color){255, 100, 100, 180});
    }
  }

  char buf[64];
  if (remaining > 0) {
    snprintf(buf, sizeof(buf), "%d", remaining);
    int textW = MeasureText(buf, 100);
    DrawText(buf, 640 - textW / 2, 220, 100, (remaining <= 5) ? RED : WHITE);
    DrawText("GET READY...", 540, 380, 30, LIGHTGRAY);
  } else {
    DrawText("GO!", 580, 220, 100, LIME);
  }
  DrawText("ESC to cancel", 570, 440, 20, GRAY);
}

void GameUpdateRacing(Game *game) {
  float dt = GetFrameTime();
  game->raceTime += dt;
  game->lapTime += dt;

  game->car.Update(dt);

  if (!TrackIsOnTrack(&game->track, game->car.position)) {
    game->car.speed *= 0.95f;
    game->offTrackTimer += dt;
    if (game->offTrackTimer > 2.0f) {
      game->car.position = TrackGetNearestPoint(&game->track, game->car.position);
      game->offTrackTimer = 0.0f;
    }
  } else {
    game->offTrackTimer = 0.0f;
  }

  if (game->car.drift > 0.3f && fabsf(game->car.speed) > 50.0f) {
    EffectsAddSkidMark(&game->effects, game->car.position);
    AudioPlaySkid();
  }

  EffectsUpdate(&game->effects, dt);

  for (int i = 0; i < game->aiCarCount; i++) {
    game->aiCars[i].Update(dt);
  }

  AudioUpdateEngine(game->car.speed, 400.0f);

  for (int i = 0; i < game->aiCarCount && i < 3; i++) {
    AICar *ai = &game->aiCars[i];
    float dx = game->car.position.x - ai->position.x;
    float dy = game->car.position.y - ai->position.y;
    float dist = sqrtf(dx * dx + dy * dy);
    float aiVx = cosf(ai->angle) * ai->speed;
    float aiVy = sinf(ai->angle) * ai->speed;
    float invDist = (dist > 1.0f) ? 1.0f / dist : 0.0f;
    float relSpeed = (aiVx * dx + aiVy * dy) * invDist;
    AudioUpdateAICar(i, dist, relSpeed, ai->speed);
  }

  float lerp = 5.0f * dt;
  game->camera.target.x += (game->car.position.x - game->camera.target.x) * lerp;
  game->camera.target.y += (game->car.position.y - game->camera.target.y) * lerp;

  if (IsKeyDown(KEY_F2)) {
    game->camera.zoom += 1.0f * dt;
    if (game->camera.zoom > 3.0f) game->camera.zoom = 3.0f;
  }
  if (IsKeyDown(KEY_F3)) {
    game->camera.zoom -= 1.0f * dt;
    if (game->camera.zoom < 0.3f) game->camera.zoom = 0.3f;
  }
  if (IsKeyDown(KEY_F4)) {
    game->camera.target = game->car.position;
    game->camera.zoom = 1.5f;
  }
  if (IsKeyPressed(KEY_F6)) {
    game->viewMode = (game->viewMode == VIEW_FIRST_PERSON) ? VIEW_TOP_DOWN : VIEW_FIRST_PERSON;
  }

  debugFrame++;
  if (debugFrame % 30 == 0) {
    fprintf(stderr,
      "[DBG] car=(%.1f,%.1f) cam_tgt=(%.1f,%.1f) zoom=%.2f\n",
      game->car.position.x, game->car.position.y,
      game->camera.target.x, game->camera.target.y, game->camera.zoom);
  }

  if (IsKeyPressed(KEY_ESCAPE)) {
    game->state = GAME_MENU;
  }
}

void GameDrawRacing(Game *game) {
  if (game->viewMode == VIEW_FIRST_PERSON) {
    FirstPersonDraw(game);
  } else {
    BeginMode2D(game->camera);
    DrawRectangleRec((Rectangle){0, 0, 2000, 2000}, DARKGREEN);
    TrackDraw(&game->track);
    EffectsDraw(&game->effects);
    for (int i = 0; i < game->aiCarCount; i++) {
      game->aiCars[i].Draw();
    }
    game->car.Draw();
    EndMode2D();

    HudDraw(game);
  }

  if (IsKeyDown(KEY_F5)) {
    Vector2 sp = GetWorldToScreen2D(game->car.position, game->camera);
    DrawLine((int)sp.x - 30, (int)sp.y, (int)sp.x + 30, (int)sp.y, YELLOW);
    DrawLine((int)sp.x, (int)sp.y - 30, (int)sp.x, (int)sp.y + 30, YELLOW);
    DrawCircle((int)sp.x, (int)sp.y, 5, YELLOW);
  }

  if (IsKeyDown(KEY_F1)) {
    DrawText("=== DEBUG ===", 10, 10, 20, LIME);
    char buf[128];
    snprintf(buf, sizeof(buf), "car:    (%.1f, %.1f)", game->car.position.x, game->car.position.y);
    DrawText(buf, 10, 35, 20, LIME);
    snprintf(buf, sizeof(buf), "speed:  %.1f", game->car.speed);
    DrawText(buf, 10, 60, 20, LIME);
    snprintf(buf, sizeof(buf), "angle:  %.2f", game->car.angle);
    DrawText(buf, 10, 85, 20, LIME);
  }

  DrawText("F2:zoom+ F3:zoom- F4:lockCam F5:crosshair", 10, 690, 16, GRAY);
}

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