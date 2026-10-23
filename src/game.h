#ifndef GAME_H
#define GAME_H

 #include "raylib.h"
#include "car.h"
#include "track.h"
#include "effects.h"
#include "ai_car.h"

// Game states
typedef enum {
  GAME_MENU,
  GAME_RACING,
  GAME_FINISHED,
} GameState;

// Game struct
typedef struct {
  GameState state;
  Car car;
  Track track;
  Effects effects;
  AICar aiCars[MAX_AI_CARS];
  int aiCarCount;
  int lap;
  int totalLaps;
  float lapTime;
  float bestLap;
  float raceTime;
  Camera2D camera;
} Game;

// State transition
void GameNextState(Game *game);

// Update functions
void GameUpdateMenu(Game *game);
void GameUpdateRacing(Game *game);
void GameUpdateFinished(Game *game);

// Draw functions
void GameDrawMenu(Game *game);
void GameDrawRacing(Game *game);
void GameDrawFinished(Game *game);

#endif // GAME_H