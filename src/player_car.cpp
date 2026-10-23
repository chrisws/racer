#include "player_car.h"
#include <math.h>

// Physics constants
#define CAR_MAX_SPEED      400.0f
#define CAR_ACCEL          300.0f
#define CAR_BRAKE_DECEL    500.0f
#define CAR_FRICTION       0.97f
#define CAR_TURN_RATE      2.5f
#define CAR_MIN_TURN_SPEED 30.0f
#define CAR_DRIFT_FACTOR   0.92f

void PlayerCar::Update(float dt) {
  // --- Input ---
  float throttle = 0.0f;
  float brake = 0.0f;
  steering = 0.0f;

  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
    throttle = 1.0f;
  }
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
    brake = 1.0f;
  }
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
    steering = 1.0f;
  }
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
    steering = -1.0f;
  }

  // --- Acceleration / Braking ---
  if (throttle > 0.0f) {
    speed += CAR_ACCEL * dt;
  }
  if (brake > 0.0f) {
    if (speed > 0.0f) {
      speed -= CAR_BRAKE_DECEL * dt;
      if (speed < 0.0f) speed = 0.0f;
    } else {
      speed -= CAR_ACCEL * 0.5f * dt;
    }
  }

  // --- Friction ---
  speed *= powf(CAR_FRICTION, dt * 60.0f);

  // Clamp speed
  if (speed > CAR_MAX_SPEED) speed = CAR_MAX_SPEED;
  if (speed < -CAR_MAX_SPEED * 0.3f) speed = -CAR_MAX_SPEED * 0.3f;

  // --- Steering ---
  float speedFactor = 0.0f;
  float absSpeed = fabsf(speed);
  if (absSpeed > CAR_MIN_TURN_SPEED) {
    speedFactor = absSpeed / CAR_MAX_SPEED;
  }

  float turnAmount = steering * CAR_TURN_RATE * speedFactor * dt;
  if (speed < 0.0f) turnAmount = -turnAmount;
  angle += turnAmount;

  // --- Drift ---
  float driftInput = fabsf(steering) * speedFactor;
  drift = drift * CAR_DRIFT_FACTOR + driftInput * (1.0f - CAR_DRIFT_FACTOR);
  if (drift > 1.0f) drift = 1.0f;

  // --- Position update ---
  float slideAngle = angle + steering * drift * 0.3f;
  float effectiveAngle = angle * (1.0f - drift * 0.5f) + slideAngle * (drift * 0.5f);

  position.x += cosf(effectiveAngle) * speed * dt;
  position.y += sinf(effectiveAngle) * speed * dt;
}