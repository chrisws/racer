#include "car.h"
#include <math.h>

void Car::DrawRotatedRect(Vector2 center, float w, float h, float angleRad, Color col) {
  float hw = w / 2.0f;
  float hh = h / 2.0f;
  float cosA = cosf(angleRad);
  float sinA = sinf(angleRad);

  // Four corners
  float ox[4] = { -hw,  hw,  hw, -hw };
  float oy[4] = { -hh, -hh,  hh,  hh };

  Vector2 c[4];
  for (int i = 0; i < 4; i++) {
    c[i].x = center.x + ox[i] * cosA - oy[i] * sinA;
    c[i].y = center.y + ox[i] * sinA + oy[i] * cosA;
  }

  // Fill with parallel thick lines (DrawTriangle unreliable in this build)
  int N = 8;
  float thick = h / N;
  for (int i = 0; i < N; i++) {
    float t = (float)i / (float)N;
    Vector2 a = { c[0].x + (c[3].x - c[0].x) * t,
                  c[0].y + (c[3].y - c[0].y) * t };
    Vector2 b = { c[1].x + (c[2].x - c[1].x) * t,
                  c[1].y + (c[2].y - c[1].y) * t };
    DrawLineEx(a, b, thick, col);
  }
}

void Car::Draw() {
  float cw = 44.0f;
  float ch = 24.0f;

  // Body
  DrawRotatedRect(position, cw, ch, angle, color);

  // Windshield (front section, slightly inset)
  Color windCol = Fade(color, 0.4f);
  Vector2 windCenter = {
    position.x + cosf(angle) * cw * 0.15f,
    position.y + sinf(angle) * cw * 0.15f,
  };
  DrawRotatedRect(windCenter, cw * 0.25f, ch * 0.6f, angle, windCol);

  // Wheels (4 small dark rects)
  Color wheelCol = BLACK;
  float ww = 10.0f, wh = 6.0f;
  float wx = cw * 0.3f;   // wheel x offset from center
  float wy = ch * 0.55f;  // wheel y offset from center

  // Wheel corner offsets in car-local space
  float woffs[4][2] = {
    { -wx, -wy },   // rear-left
    { -wx,  wy },   // rear-right
    {  wx, -wy },   // front-left
    {  wx,  wy },   // front-right
  };

  float cosA = cosf(angle);
  float sinA = sinf(angle);
  for (int i = 0; i < 4; i++) {
    Vector2 wc = {
      position.x + woffs[i][0] * cosA - woffs[i][1] * sinA,
      position.y + woffs[i][0] * sinA + woffs[i][1] * cosA,
    };
    DrawRotatedRect(wc, ww, wh, angle, wheelCol);
  }
}