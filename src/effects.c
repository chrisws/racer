#include "effects.h"

void EffectsInit(Effects *fx) {
  fx->count = 0;
  for (int i = 0; i < MAX_SKID_MARKS; i++) {
    fx->marks[i].age = 0.0f;
    fx->marks[i].maxAge = 3.0f;
  }
}

void EffectsUpdate(Effects *fx, float dt) {
  for (int i = 0; i < fx->count; i++) {
    fx->marks[i].age += dt;
    if (fx->marks[i].age >= fx->marks[i].maxAge) {
      // Remove by swapping with last
      fx->marks[i] = fx->marks[fx->count - 1];
      fx->count--;
      i--;
    }
  }
}

void EffectsAddSkidMark(Effects *fx, Vector2 pos) {
  if (fx->count >= MAX_SKID_MARKS) return;
  fx->marks[fx->count].pos = pos;
  fx->marks[fx->count].age = 0.0f;
  fx->marks[fx->count].maxAge = 3.0f;
  fx->count++;
}

void EffectsDraw(const Effects *fx) {
  for (int i = 0; i < fx->count; i++) {
    float alpha = 1.0f - (fx->marks[i].age / fx->marks[i].maxAge);
    if (alpha < 0.0f) alpha = 0.0f;
    Color c = { 40, 40, 40, (int)(255 * alpha) };
    DrawCircleV(fx->marks[i].pos, 3.0f, c);
  }
}