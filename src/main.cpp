#include "game.h"
#include "audio.h"
#include <stdio.h>

static void InitGame(Game *game) {
  game->state = GAME_MENU;
  game->lap = 0;
  game->totalLaps = 3;
  game->lapTime = 0.0f;
  game->bestLap = 0.0f;
  game->raceTime = 0.0f;
  game->countdownTimer = 0.0f;
  game->car = PlayerCar((Vector2){400.0f, 900.0f}, 0.0f);
  TrackInit(&game->track);
  EffectsInit(&game->effects);
  game->aiCarCount = MAX_AI_CARS;
  AICar::InitAll(game->aiCars, game->aiCarCount, &game->track);
  game->camera = (Camera2D){
    .offset = (Vector2){640.0f, 360.0f},
    .target = (Vector2){400.0f, 900.0f},
    .rotation = 0.0f,
    .zoom = 1.0f,
  };
}

int main(void) {
  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(1280, 720, "Racer");
  SetTargetFPS(60);
  AudioInit();

  Game game;
  InitGame(&game);

  while (!WindowShouldClose()) {
    switch (game.state) {
      case GAME_MENU:     GameUpdateMenu(&game); break;
      case GAME_COUNTDOWN: GameUpdateCountdown(&game); break;
      case GAME_RACING:   GameUpdateRacing(&game); break;
      case GAME_FINISHED: GameUpdateFinished(&game); break;
    }

    BeginDrawing();
    ClearBackground(BLACK);

    switch (game.state) {
      case GAME_MENU:     GameDrawMenu(&game); break;
      case GAME_COUNTDOWN: GameDrawCountdown(&game); break;
      case GAME_RACING:   GameDrawRacing(&game); break;
      case GAME_FINISHED: GameDrawFinished(&game); break;
    }

    EndDrawing();
  }

  AudioShutdown();
  CloseWindow();
  return 0;
}