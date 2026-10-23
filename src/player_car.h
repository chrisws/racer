#ifndef PLAYER_CAR_H
#define PLAYER_CAR_H

#include "car.h"

class PlayerCar : public Car {
public:
  float steering;   // -1, 0, 1
  float drift;      // 0..1 lateral slip

  PlayerCar() : Car(), steering(0.0f), drift(0.0f) {}
  PlayerCar(Vector2 pos, float ang) : Car(pos, ang, RED), steering(0.0f), drift(0.0f) {}

  void Update(float dt) override;
};

#endif // PLAYER_CAR_H