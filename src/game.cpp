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
    game->fuel = 100.0f;
    for (int i = 0; i < 4; i++) game->tirePressure[i] = 100.0f;
    game->warningLights = WARN_NONE;
    game->warningTimer = 5.0f;
    game->boostTimer = 0.0f;
    game->hypersonicTimer = 0.0f;
    game->inPitStop = false;
    game->pitStopTimer = 0.0f;
    game->pitStopCooldown = 0.0f;
    game->engineTuning = 0.5f;
    for (int i = 0; i < MAX_BOOST_COINS; i++) game->boostCollected[i] = 0;
    game->lastTrackIdx = 0;
    game->kangarooTimer = 8.0f;
    game->kangarooJumpT = 0.0f;
    game->kangarooActive = 0;
    game->kangarooHit = 0;
    game->windshieldBroken = 0;
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

  // --- Pit stop handling ---
  if (game->inPitStop) {
    game->pitStopTimer -= dt;
    game->car.speed = 0.0f;
    if (IsKeyDown(KEY_LEFT)) {
      game->engineTuning -= 0.5f * dt;
      if (game->engineTuning < 0.0f) game->engineTuning = 0.0f;
    }
    if (IsKeyDown(KEY_RIGHT)) {
      game->engineTuning += 0.5f * dt;
      if (game->engineTuning > 1.0f) game->engineTuning = 1.0f;
    }
    if (IsKeyPressed(KEY_SPACE) || game->pitStopTimer <= 0.0f) {
      game->inPitStop = false;
      game->pitStopCooldown = 2.0f; // 2s cooldown before pit can re-trigger
      for (int i = 0; i < 4; i++) game->tirePressure[i] = 100.0f;
    }
    EffectsUpdate(&game->effects, dt);
    return;
  }

  game->car.Update(dt);

  bool onTrack = TrackIsOnTrack(&game->track, game->car.position);

  if (!onTrack) {
    game->car.speed *= 0.95f;
    game->offTrackTimer += dt;
    if (game->offTrackTimer > 2.0f) {
      game->car.position = TrackGetNearestPoint(&game->track, game->car.position);
      game->offTrackTimer = 0.0f;
    }
  } else {
    game->offTrackTimer = 0.0f;
  }

  // --- Fuel drain ---
  float speedNorm = fabsf(game->car.speed) / 400.0f;
  float fuelDrain = 0.3f + speedNorm * 1.2f; // base + speed
  if (!onTrack) fuelDrain *= 2.0f;           // off-track penalty
  if (speedNorm > 0.8f) fuelDrain *= 1.5f;   // speeding penalty
  game->fuel -= fuelDrain * dt;
  if (game->fuel < 0.0f) game->fuel = 0.0f;
  if (game->fuel <= 0.0f) {
    game->car.speed *= 0.9f; // sputter to a stop
    if (game->car.speed < 1.0f) {
      game->state = GAME_FINISHED;
      return;
    }
  }

  // --- Tire pressure drain ---
  float tireDrain = 0.15f + speedNorm * 0.3f;
  if (!onTrack) tireDrain *= 2.5f;
  if (game->car.drift > 0.3f) tireDrain *= 1.8f;
  for (int i = 0; i < 4; i++) {
    game->tirePressure[i] -= tireDrain * dt;
    if (game->tirePressure[i] < 0.0f) game->tirePressure[i] = 0.0f;
  }
  // Low tires reduce effective speed
  float avgTire = (game->tirePressure[0] + game->tirePressure[1] +
                   game->tirePressure[2] + game->tirePressure[3]) / 4.0f;
  if (avgTire < 30.0f) {
    game->car.speed *= (0.8f + 0.2f * avgTire / 30.0f);
  }

  // --- Random warning lights ---
  game->warningTimer -= dt;
  if (game->warningTimer <= 0.0f) {
    game->warningTimer = 4.0f + (float)GetRandomValue(0, 6);
    int roll = GetRandomValue(0, 100);
    if (roll < 20) game->warningLights |= WARN_OVERHEAT;
    else if (roll < 35) game->warningLights |= WARN_OIL;
    else if (roll < 45) game->warningLights |= WARN_SPARK;
    // Auto-clear after a while
    game->warningLights &= ~WARN_FUEL;
    if (game->fuel < 20.0f) game->warningLights |= WARN_FUEL;
  }
  // Clear random warnings after 8s
  if (game->warningTimer > 8.0f) {
    game->warningLights &= ~(WARN_OVERHEAT | WARN_OIL | WARN_SPARK);
  }

  // --- Boost coin collection ---
  int N = game->track.pointCount;
  for (int i = 0; i < game->boostCoinCount; i++) {
    if (game->boostCollected[i]) continue;
    Vector2 coinPos = game->track.points[game->boostCoins[i] % N];
    float dx = game->car.position.x - coinPos.x;
    float dy = game->car.position.y - coinPos.y;
    if (dx * dx + dy * dy < 400.0f) { // 20 unit radius
      game->boostCollected[i] = 1;
      game->boostTimer = 3.0f; // 3 second boost
      AudioPlayChime();
    }
  }

  // --- Lap detection ---
  int curSeg = TrackGetNearestSegment(&game->track, game->car.position);
  int prevSeg = game->lastTrackIdx;
  game->lastTrackIdx = curSeg;
  // Detect crossing start/finish: high segment → low segment (wrap)
  if (prevSeg > N / 2 && curSeg < N / 4) {
    game->lap++;
    if (game->lapTime > 0.0f) {
      if (game->bestLap == 0.0f || game->lapTime < game->bestLap) {
        game->bestLap = game->lapTime;
      }
    }
    game->lapTime = 0.0f;
    if (game->lap >= game->totalLaps) {
      game->state = GAME_FINISHED;
      return;
    }
  }

  // --- Hypersonic / Nitro strip ---
  Vector2 hypPos = game->track.points[game->hypersonicIdx % N];
  float hdx = game->car.position.x - hypPos.x;
  float hdy = game->car.position.y - hypPos.y;
  if (hdx * hdx + hdy * hdy < 625.0f) { // 25 unit radius
    if (game->hypersonicTimer <= 0.0f) {
      game->hypersonicTimer = 5.0f; // 5 second hypersonic boost
      game->car.speed += 100.0f;    // immediate acceleration kick
      if (game->car.speed > 700.0f) game->car.speed = 700.0f;
    }
  }
  if (game->hypersonicTimer > 0.0f) game->hypersonicTimer -= dt;

  // --- Pit stop zone ---
  if (game->pitStopCooldown > 0.0f) {
    game->pitStopCooldown -= dt;
  }
  Vector2 pitPos = game->track.points[game->pitZoneIdx % N];
  float pdx = game->car.position.x - pitPos.x;
  float pdy = game->car.position.y - pitPos.y;
  if (pdx * pdx + pdy * pdy < 900.0f && game->car.speed < 50.0f && game->pitStopCooldown <= 0.0f) { // 30 unit radius
    game->inPitStop = true;
    game->pitStopTimer = PIT_STOP_DURATION;
  }

  // --- Boost timers affect max speed ---
  if (game->boostTimer > 0.0f) game->boostTimer -= dt;
  float effectiveMax = 400.0f;
  if (game->boostTimer > 0.0f) effectiveMax = 550.0f;
  if (game->hypersonicTimer > 0.0f) effectiveMax = 700.0f;
  if (game->car.speed > effectiveMax) game->car.speed = effectiveMax;

  // --- Drift effects ---
  if (game->car.drift > 0.3f && fabsf(game->car.speed) > 50.0f) {
    EffectsAddSkidMark(&game->effects, game->car.position);
    AudioPlaySkid();
  }

  EffectsUpdate(&game->effects, dt);

  for (int i = 0; i < game->aiCarCount; i++) {
    game->aiCars[i].Update(dt);
  }

  AudioUpdateEngine(game->car.speed, effectiveMax);

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
      "[DBG] car=(%.1f,%.1f) fuel=%.1f boost=%.1f hyp=%.1f\n",
      game->car.position.x, game->car.position.y,
      game->fuel, game->boostTimer, game->hypersonicTimer);
  }

  // --- Kangaroo ---
  if (!game->kangarooActive) {
    game->kangarooTimer -= dt;
    if (game->kangarooTimer <= 0.0f) {
      // Spawn kangaroo: pick a random track segment ahead of the player
      int spawnSeg = (curSeg + N / 4 + GetRandomValue(0, N / 4)) % N;
      Vector2 sp = game->track.points[spawnSeg];
      Vector2 spNext = game->track.points[(spawnSeg + 1) % N];
      float sdx = spNext.x - sp.x;
      float sdy = spNext.y - sp.y;
      float slen = sqrtf(sdx * sdx + sdy * sdy);
      if (slen > 0.001f) {
        float spx = -sdy / slen;
        float spy = sdx / slen;
        // Start on one side, jump to the other
        float side = (GetRandomValue(0, 1) == 0) ? 1.0f : -1.0f;
        float offset = game->track.halfWidth * 2.0f;
        game->kangarooPos.x = sp.x + spx * offset * side;
        game->kangarooPos.y = sp.y + spy * offset * side;
        // Store target (other side)
        game->kangarooPos.x += spx * offset * 2.0f * (-side);
        game->kangarooPos.y += spy * offset * 2.0f * (-side);
        // Actually, let's store start and compute jump in draw
        // For simplicity: kangarooPos is the landing spot, jump arc is computed
        game->kangarooActive = 1;
        game->kangarooJumpT = 0.0f;
      }
    }
  } else {
    game->kangarooJumpT += dt * 1.5f; // jump lasts ~0.67s
    if (game->kangarooJumpT >= 1.0f) {
      game->kangarooActive = 0;
      game->kangarooTimer = 10.0f + (float)GetRandomValue(0, 10);
    }
    // Check collision with player car
    float kdx = game->car.position.x - game->kangarooPos.x;
    float kdy = game->car.position.y - game->kangarooPos.y;
    if (kdx * kdx + kdy * kdy < 900.0f) { // 30 unit radius
      if (!game->kangarooHit) {
        game->kangarooHit = 1;
        game->windshieldBroken = 1;
        game->state = GAME_FINISHED;
        return;
      }
    }
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
  if (IsKeyPressed(KEY_R)) {
    // Restart race directly
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
    game->fuel = 100.0f;
    for (int i = 0; i < 4; i++) game->tirePressure[i] = 100.0f;
    game->warningLights = WARN_NONE;
    game->warningTimer = 5.0f;
    game->boostTimer = 0.0f;
    game->hypersonicTimer = 0.0f;
    game->inPitStop = false;
    game->pitStopTimer = 0.0f;
    game->pitStopCooldown = 0.0f;
    game->engineTuning = 0.5f;
    for (int i = 0; i < MAX_BOOST_COINS; i++) game->boostCollected[i] = 0;
    game->lastTrackIdx = 0;
    game->kangarooTimer = 8.0f;
    game->kangarooJumpT = 0.0f;
    game->kangarooActive = 0;
    game->kangarooHit = 0;
    game->windshieldBroken = 0;
    lastCountdownSec = -1;
    AudioPlayStartBeep();
  }
}

void GameDrawFinished(Game *game) {
  if (game->windshieldBroken) {
    DrawText("KANGAROO HIT!", 400, 150, 60, RED);
    DrawText("Windshield shattered!", 460, 250, 30, ORANGE);
  } else if (game->fuel <= 0.0f) {
    DrawText("OUT OF FUEL!", 420, 150, 60, RED);
    DrawText("Engine stalled...", 480, 250, 30, ORANGE);
  } else {
    DrawText("RACE COMPLETE!", 440, 150, 60, GOLD);
  }

  char buf[128];
  snprintf(buf, sizeof(buf), "Total Time: %.2f", game->raceTime);
  DrawText(buf, 520, 320, 30, WHITE);
  snprintf(buf, sizeof(buf), "Best Lap: %.2f", game->bestLap);
  DrawText(buf, 520, 370, 30, WHITE);
  snprintf(buf, sizeof(buf), "Laps: %d / %d", game->lap, game->totalLaps);
  DrawText(buf, 520, 420, 30, WHITE);
  DrawText("Press ENTER for menu", 500, 500, 20, LIGHTGRAY);
  DrawText("Press R to restart", 510, 540, 20, LIME);
}