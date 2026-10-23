#ifndef CAR_H
#define CAR_H

#include "raylib.h"

// Base car class - shared position, angle, speed, and drawing
class Car {
public:
  Vector2 position;
  float angle;    // radians, 0 = right
  float speed;
  Color color;

  Car() : position{0,0}, angle(0.0f), speed(0.0f), color(WHITE) {}
  Car(Vector2 pos, float ang, Color col)
      : position(pos), angle(ang), speed(0.0f), color(col) {}

  virtual ~Car() {}

  // Draw the car (body, windshield, wheels) at current position/angle
  virtual void Draw();

  // Per-frame update (override in derived)
  virtual void Update(float dt) = 0;

  // Helper: draw a rotated rectangle using two triangles
  static void DrawRotatedRect(Vector2 center, float w, float h, float angleRad, Color col);
};

#endif // CAR_H