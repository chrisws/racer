#include "ai_car.h"
#include <math.h>

void AICar::InitAll(AICar *cars, int count, const Track *track) {
  static const Vector2 gridPos[] = {
    { 500.0f, 870.0f },
    { 500.0f, 930.0f },
    { 600.0f, 900.0f },
  };
  static const Color gridColors[] = { BLUE, GREEN, ORANGE };
  static const float gridSpeeds[] = { 180.0f, 200.0f, 220.0f };

  for (int i = 0; i < count; i++) {
    cars[i] = AICar(gridPos[i], 0.0f, gridColors[i], gridSpeeds[i]);
    cars[i].track = track;
  }
}

void AICar::Update(float dt) {
  int curIdx = waypointIndex;
  int nextIdx = (curIdx + 1) % track->pointCount;
  Vector2 curWp = track->points[curIdx];
  Vector2 nextWp = track->points[nextIdx];

  Vector2 target;
  target.x = curWp.x + (nextWp.x - curWp.x) * 0.4f;
  target.y = curWp.y + (nextWp.y - curWp.y) * 0.4f;

  float dx = target.x - position.x;
  float dy = target.y - position.y;
  float targetAngle = atan2f(dy, dx);

  float angleDiff = targetAngle - angle;
  while (angleDiff > PI) angleDiff -= 2.0f * PI;
  while (angleDiff < -PI) angleDiff += 2.0f * PI;

  float maxTurn = 4.0f * dt;
  if (angleDiff > maxTurn) angleDiff = maxTurn;
  if (angleDiff < -maxTurn) angleDiff = -maxTurn;
  angle += angleDiff;

  float targetSpd = targetSpeed;
  if (fabsf(angleDiff) > 0.6f) {
    targetSpd *= 0.35f;
  } else if (fabsf(angleDiff) > 0.3f) {
    targetSpd *= 0.6f;
  }

  if (speed < targetSpd) {
    speed += 200.0f * dt;
    if (speed > targetSpd) speed = targetSpd;
  } else {
    speed -= 150.0f * dt;
    if (speed < targetSpd) speed = targetSpd;
  }

  position.x += cosf(angle) * speed * dt;
  position.y += sinf(angle) * speed * dt;

  if (!TrackIsOnTrack(track, position)) {
    position = TrackGetNearestPoint(track, position);
    angle = targetAngle;
    speed *= 0.7f;
  }

  float wdx = curWp.x - position.x;
  float wdy = curWp.y - position.y;
  float dist = sqrtf(wdx * wdx + wdy * wdy);
  if (dist < 50.0f) {
    waypointIndex = nextIdx;
    if (waypointIndex == 1) {
      lap++;
    }
  }
}