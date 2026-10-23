#include "ai_car.h"
#include <math.h>

void AICarsInit(AICar *cars, int count, const Track *track) {
  // Starting grid positions near the start/finish line
  static const Vector2 gridPos[] = {
    { 500.0f, 870.0f },
    { 500.0f, 930.0f },
    { 600.0f, 900.0f },
  };
  static const Color gridColors[] = { BLUE, GREEN, ORANGE };
  static const float gridSpeeds[] = { 180.0f, 200.0f, 220.0f };

  for (int i = 0; i < count; i++) {
    cars[i].position = gridPos[i];
    cars[i].angle = 0.0f;  // facing right along the start straight
    cars[i].speed = 0.0f;
    cars[i].targetSpeed = gridSpeeds[i];
    cars[i].waypointIndex = 1;  // head toward waypoint 1
    cars[i].color = gridColors[i];
    cars[i].lap = 0;
  }
}

void AICarUpdate(AICar *car, float dt, const Track *track) {
  // Blended target: current waypoint + 40% toward next (smoother cornering)
  int curIdx = car->waypointIndex;
  int nextIdx = (curIdx + 1) % track->pointCount;
  Vector2 curWp = track->points[curIdx];
  Vector2 nextWp = track->points[nextIdx];

  Vector2 target;
  target.x = curWp.x + (nextWp.x - curWp.x) * 0.4f;
  target.y = curWp.y + (nextWp.y - curWp.y) * 0.4f;

  float dx = target.x - car->position.x;
  float dy = target.y - car->position.y;
  float targetAngle = atan2f(dy, dx);

  // Normalise angle difference to -PI..PI
  float angleDiff = targetAngle - car->angle;
  while (angleDiff > PI) angleDiff -= 2.0f * PI;
  while (angleDiff < -PI) angleDiff += 2.0f * PI;

  // Smooth steering with a max turn rate
  float maxTurn = 4.0f * dt;
  if (angleDiff > maxTurn) angleDiff = maxTurn;
  if (angleDiff < -maxTurn) angleDiff = -maxTurn;
  car->angle += angleDiff;

  // Slow down for tight turns (based on blended target angle)
  float targetSpd = car->targetSpeed;
  if (fabsf(angleDiff) > 0.6f) {
    targetSpd *= 0.35f;
  } else if (fabsf(angleDiff) > 0.3f) {
    targetSpd *= 0.6f;
  }

  // Accelerate / decelerate toward target speed
  if (car->speed < targetSpd) {
    car->speed += 200.0f * dt;
    if (car->speed > targetSpd) car->speed = targetSpd;
  } else {
    car->speed -= 150.0f * dt;
    if (car->speed < targetSpd) car->speed = targetSpd;
  }

  // Move
  car->position.x += cosf(car->angle) * car->speed * dt;
  car->position.y += sinf(car->angle) * car->speed * dt;

  // Clamp to track: if off-track, nudge back gently
  if (!TrackIsOnTrack(track, car->position)) {
    Vector2 nearest = TrackGetNearestPoint(track, car->position);
    car->position = nearest;
    car->angle = targetAngle;  // face the blended target
    car->speed *= 0.7f;  // gentle penalty (not 0.5 which causes oscillation)
  }

  // Advance to next waypoint (larger threshold for stability)
  float wdx = curWp.x - car->position.x;
  float wdy = curWp.y - car->position.y;
  float dist = sqrtf(wdx * wdx + wdy * wdy);
  if (dist < 50.0f) {
    car->waypointIndex = nextIdx;
    if (car->waypointIndex == 1) {
      car->lap++;
    }
  }
}

void AICarDraw(const AICar *car) {
  float carW = 40.0f;
  float carH = 22.0f;
  float deg = car->angle * 180.0f / PI;

  // Body
  Rectangle body = { -carW / 2, -carH / 2, carW, carH };
  DrawRectanglePro(body, car->position, deg, car->color);

  // Windshield (front section)
  Rectangle wind = { carW * 0.1f, -carH * 0.3f, carW * 0.25f, carH * 0.6f };
  DrawRectanglePro(wind, car->position, deg, Fade(car->color, 0.4f));

  // Wheels (4 small dark rectangles)
  Color wheelColor = BLACK;
  float ww = 8.0f, wh = 5.0f;
  Rectangle wtl = { -carW * 0.3f, -carH * 0.55f, ww, wh };
  Rectangle wtr = { -carW * 0.3f, carH * 0.55f - wh, ww, wh };
  Rectangle wbl = { carW * 0.3f - ww, -carH * 0.55f, ww, wh };
  Rectangle wbr = { carW * 0.3f - ww, carH * 0.55f - wh, ww, wh };
  DrawRectanglePro(wtl, car->position, deg, wheelColor);
  DrawRectanglePro(wtr, car->position, deg, wheelColor);
  DrawRectanglePro(wbl, car->position, deg, wheelColor);
  DrawRectanglePro(wbr, car->position, deg, wheelColor);
}