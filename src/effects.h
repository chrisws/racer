#ifndef EFFECTS_H
#define EFFECTS_H

#include "raylib.h"

#define MAX_SKID_MARKS 128

typedef struct {
  Vector2 pos;
  float age;
  float maxAge;
} SkidMark;

typedef struct {
  SkidMark marks[MAX_SKID_MARKS];
  int count;
} Effects;

void EffectsInit(Effects *fx);
void EffectsUpdate(Effects *fx, float dt);
void EffectsAddSkidMark(Effects *fx, Vector2 pos);
void EffectsDraw(const Effects *fx);

#endif // EFFECTS_H