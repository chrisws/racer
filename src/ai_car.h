#ifndef AI_CAR_H
#define AI_CAR_H

#include "raylib.h"
#include "track.h"

#define MAX_AI_CARS 3

typedef struct {
  Vector2 position;
  float angle;
  float speed;
  float targetSpeed;
  int waypointIndex;
  Color color;
  int lap;
} AICar;

void AICarsInit(AICar *cars, int count, const Track *track);
void AICarUpdate(AICar *car, float dt, const Track *track);
void AICarDraw(const AICar *car);

#endif // AI_CAR_H