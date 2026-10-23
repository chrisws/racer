#ifndef CAR_H
#define CAR_H

#include "raylib.h"

typedef struct {
  Vector2 position;
  float angle;       // radians, 0 = right
  float speed;       // current speed (pixels/s)
  float steering;    // -1, 0, 1
  float drift;       // 0..1 lateral slip
} Car;

void CarInit(Car *car, Vector2 pos, float angle);
void CarUpdate(Car *car, float dt);

#endif // CAR_H