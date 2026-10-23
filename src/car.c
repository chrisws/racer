#include "car.h"
#include <math.h>

// Physics constants
#define CAR_MAX_SPEED     400.0f
#define CAR_ACCEL         300.0f
#define CAR_BRAKE_DECEL   500.0f
#define CAR_FRICTION      0.97f
#define CAR_TURN_RATE     2.5f    // radians/s at full speed
#define CAR_MIN_TURN_SPEED 30.0f  // min speed to turn
#define CAR_DRIFT_FACTOR  0.92f   // how quickly drift decays

void CarInit(Car *car, Vector2 pos, float angle) {
  car->position = pos;
  car->angle = angle;
  car->speed = 0.0f;
  car->steering = 0.0f;
  car->drift = 0.0f;
}

void CarUpdate(Car *car, float dt) {
  // --- Input ---
  float throttle = 0.0f;
  float brake = 0.0f;
  car->steering = 0.0f;

  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
    throttle = 1.0f;
  }
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
    brake = 1.0f;
  }
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
    car->steering = -1.0f;
  }
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
    car->steering = 1.0f;
  }

  // --- Acceleration / Braking ---
  if (throttle > 0.0f) {
    car->speed += CAR_ACCEL * dt;
  }
  if (brake > 0.0f) {
    if (car->speed > 0.0f) {
      car->speed -= CAR_BRAKE_DECEL * dt;
      if (car->speed < 0.0f) car->speed = 0.0f;
    } else {
      // Reverse
      car->speed -= CAR_ACCEL * 0.5f * dt;
    }
  }

  // --- Friction (frame-rate independent) ---
  car->speed *= powf(CAR_FRICTION, dt * 60.0f);

  // Clamp speed
  if (car->speed > CAR_MAX_SPEED) car->speed = CAR_MAX_SPEED;
  if (car->speed < -CAR_MAX_SPEED * 0.3f) car->speed = -CAR_MAX_SPEED * 0.3f;

  // --- Steering ---
  // Turn rate scales with speed (can't turn while stationary)
  float speedFactor = 0.0f;
  float absSpeed = fabsf(car->speed);
  if (absSpeed > CAR_MIN_TURN_SPEED) {
    speedFactor = absSpeed / CAR_MAX_SPEED;
  }

  float turnAmount = car->steering * CAR_TURN_RATE * speedFactor * dt;
  // Reversing flips steering direction
  if (car->speed < 0.0f) turnAmount = -turnAmount;
  car->angle += turnAmount;

  // --- Drift ---
  // Drift increases when turning hard at speed
  float driftInput = fabsf(car->steering) * speedFactor;
  car->drift = car->drift * CAR_DRIFT_FACTOR + driftInput * (1.0f - CAR_DRIFT_FACTOR);
  if (car->drift > 1.0f) car->drift = 1.0f;

  // --- Position update ---
  // Effective direction blends facing and velocity (drift effect)
  float effectiveAngle = car->angle;
  // When drifting, the car slides slightly outward
  float slideAngle = car->angle + car->steering * car->drift * 0.3f;
  effectiveAngle = car->angle * (1.0f - car->drift * 0.5f) + slideAngle * (car->drift * 0.5f);

  car->position.x += cosf(effectiveAngle) * car->speed * dt;
  car->position.y += sinf(effectiveAngle) * car->speed * dt;
}