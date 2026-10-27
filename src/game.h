#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "car.h"
#include "player_car.h"
#include "ai_car.h"
#include "track.h"
#include "effects.h"

#define COUNTDOWN_DURATION 5.0f

typedef enum {
  GAME_MENU,
  GAME_COUNTDOWN,
  GAME_RACING,
  GAME_FINISHED,
} GameState;

typedef enum {
  VIEW_FIRST_PERSON,
  VIEW_TOP_DOWN,
} ViewMode;

#define MAX_BOOST_COINS 8
#define PIT_STOP_DURATION 3.0f

typedef enum {
  WARN_NONE = 0,
  WARN_OVERHEAT = 1,
  WARN_OIL = 2,
  WARN_SPARK = 4,
  WARN_FUEL = 8,
} WarningLight;

struct Game {
  GameState state;
  ViewMode viewMode;
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
  float offTrackTimer;
  Camera2D camera;

  // Fuel
  float fuel;

  // Tires (FL, FR, RL, RR)
  float tirePressure[4];

  // Warning lights (bitmask)
  int warningLights;
  float warningTimer;

  // Turbo boost
  float boostTimer;
  int boostCoins[MAX_BOOST_COINS];
  int boostCoinCount;
  int boostCollected[MAX_BOOST_COINS];

  // Hypersonic line
  int hypersonicIdx;
  float hypersonicTimer;

  // Pit stop
  int pitZoneIdx;
  bool inPitStop;
  float pitStopTimer;
  float pitStopCooldown; // prevents immediate re-trigger after exit
  float engineTuning; // 0.0 - 1.0

  // Spectators
  int spectatorCount;

  // Lap tracking
  int lastTrackIdx;

  // Kangaroo
  Vector2 kangarooPos;
  float kangarooTimer;
  float kangarooJumpT;   // 0..1 jump progress
  int kangarooActive;
  int kangarooHit;

  // Windshield broken
  int windshieldBroken;
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