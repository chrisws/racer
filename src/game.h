#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "car.h"
#include "player_car.h"
#include "ai_car.h"
#include "track.h"
#include "effects.h"

#define COUNTDOWN_DURATION 60.0f

typedef enum {
  GAME_MENU,
  GAME_COUNTDOWN,
  GAME_RACING,
  GAME_FINISHED,
} GameState;

struct Game {
  GameState state;
  PlayerCar car;
  Track track;
  Effects effects;
  AICar aiCars[MAX_AI_CARS];
  int aiCarCount;
  int lap;
  int totalLaps;
  float lapTime;
  float bestLap;
  float raceTime;
  float countdownTimer;
  Camera2D camera;
};

void GameUpdateMenu(Game *game);
void GameUpdateCountdown(Game *game);
void GameUpdateRacing(Game *game);
void GameUpdateFinished(Game *game);

void GameDrawMenu(Game *game);
void GameDrawCountdown(Game *game);
void GameDrawRacing(Game *game);
void GameDrawFinished(Game *game);

#endif // GAME_H