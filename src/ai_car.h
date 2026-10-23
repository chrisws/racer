#ifndef AI_CAR_H
#define AI_CAR_H

#include "car.h"
#include "track.h"

#define MAX_AI_CARS 3

class AICar : public Car {
public:
  float targetSpeed;
  int waypointIndex;
  int lap;
  const Track *track;

  AICar() : Car(), targetSpeed(200.0f), waypointIndex(1), lap(0), track(nullptr) {}
  AICar(Vector2 pos, float ang, Color col, float tSpeed)
      : Car(pos, ang, col), targetSpeed(tSpeed), waypointIndex(1), lap(0), track(nullptr) {}

  void Update(float dt) override;

  static void InitAll(AICar *cars, int count, const Track *track);
};

#endif // AI_CAR_H