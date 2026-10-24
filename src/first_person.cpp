#include "first_person.h"
#include <math.h>
#include <stdio.h>

// === Layout constants ===
#define WS_TOP            0
#define WS_BOTTOM         500
#define DASH_TOP          500
#define DASH_BOTTOM       720
#define HORIZON_Y         180
#define FOCAL             300.0f
#define NEAR_DIST         15.0f
#define FAR_DIST          800.0f
#define MIRROR_X          530
#define MIRROR_Y          20
#define MIRROR_W          220
#define MIRROR_H          110

// === Projection helpers ===

static void WorldToLocal(const Game *game, Vector2 world, float *lx, float *ly) {
  float dx = world.x - game->car.position.x;
  float dy = world.y - game->car.position.y;
  float c = cosf(game->car.angle);
  float s = sinf(game->car.angle);
  *lx = -dx * s + dy * c;
  *ly = dx * c + dy * s;
}

static bool Project(float lx, float ly, float steerOff, float *sx, float *sy, float *scale) {
  if (ly < 2.0f) return false;
  *scale = FOCAL / ly;
  *sx = 640.0f + lx * *scale + steerOff;
  *sy = HORIZON_Y + (WS_BOTTOM - HORIZON_Y) * (NEAR_DIST / ly);
  if (*sy > WS_BOTTOM) *sy = WS_BOTTOM;
  if (*sy < HORIZON_Y) *sy = HORIZON_Y;
  return true;
}

// Find the track center point at a given distance ahead of a position
static Vector2 TrackPointAtDistance(const Track *track, Vector2 from, float dist) {
  int N = track->pointCount;

  // Find nearest segment and projection
  int nearestSeg = 0;
  float minD = 1e9f;
  float nearestT = 0.0f;
  for (int i = 0; i < N; i++) {
    int nidx = (i + 1) % N;
    Vector2 a = track->points[i];
    Vector2 b = track->points[nidx];
    float abx = b.x - a.x;
    float aby = b.y - a.y;
    float abLen2 = abx * abx + aby * aby;
    if (abLen2 < 0.001f) continue;
    float t = ((from.x - a.x) * abx + (from.y - a.y) * aby) / abLen2;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    float px = a.x + t * abx;
    float py = a.y + t * aby;
    float dx = from.x - px;
    float dy = from.y - py;
    float d = sqrtf(dx * dx + dy * dy);
    if (d < minD) { minD = d; nearestSeg = i; nearestT = t; }
  }

  // Walk forward from the nearest point along the track
  int idx = nearestSeg;
  int nidx = (idx + 1) % N;
  Vector2 a = track->points[idx];
  Vector2 b = track->points[nidx];
  float segLen = sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
  float remaining = dist - nearestT * segLen;

  int safety = 0;
  while (remaining > 0.0f && safety < N * 2) {
    nidx = (idx + 1) % N;
    a = track->points[idx];
    b = track->points[nidx];
    segLen = sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
    if (segLen < 0.001f) { idx = nidx; safety++; continue; }
    if (remaining <= segLen) {
      float t = remaining / segLen;
      return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }
    remaining -= segLen;
    idx = nidx;
    safety++;
  }

  return track->points[idx];
}

// === Road drawing ===

static void DrawRoad(const Game *game, float steerOff) {
  const Track *track = &game->track;
  int numStrips = 50;
  float distStep = (FAR_DIST - NEAR_DIST) / numStrips;

  float lxArr[50], lyArr[50], rxArr[50], ryArr[50];
  int valid = 0;

  for (int i = 0; i < numStrips; i++) {
    float dist = NEAR_DIST + i * distStep;
    Vector2 center = TrackPointAtDistance(track, game->car.position, dist);

    // Get track direction at this point for perpendicular
    Vector2 centerNext = TrackPointAtDistance(track, game->car.position, dist + 5.0f);
    float dx = centerNext.x - center.x;
    float dy = centerNext.y - center.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) continue;
    float px = -dy / len;
    float py = dx / len;

    Vector2 lw = { center.x + px * track->halfWidth, center.y + py * track->halfWidth };
    Vector2 rw = { center.x - px * track->halfWidth, center.y - py * track->halfWidth };

    float llx, lly, rlx, rly;
    WorldToLocal(game, lw, &llx, &lly);
    WorldToLocal(game, rw, &rlx, &rly);

    float lsx, lsy, rsx, rsy, sc;
    if (Project(llx, lly, steerOff, &lsx, &lsy, &sc) &&
        Project(rlx, rly, steerOff, &rsx, &rsy, &sc)) {
      lxArr[valid] = lsx;
      lyArr[valid] = lsy;
      rxArr[valid] = rsx;
      ryArr[valid] = rsy;
      valid++;
    }
  }

  if (valid < 2) return;

  // Draw road surface as filled strips between consecutive rows
  for (int i = 0; i < valid - 1; i++) {
    float y0 = lyArr[i];
    float y1 = lyArr[i + 1];
    float lx0 = lxArr[i];
    float rx0 = rxArr[i];
    float lx1 = lxArr[i + 1];
    float rx1 = rxArr[i + 1];
    Color col = (i % 4 < 2) ? (Color){45, 45, 48, 255} : (Color){38, 38, 42, 255};
    float top = (y0 < y1) ? y0 : y1;
    float bot = (y0 < y1) ? y1 : y0;
    float left = (lx0 < rx0) ? lx0 : rx0;
    float right = (lx0 < rx0) ? rx0 : lx0;
    float left1 = (lx1 < rx1) ? lx1 : rx1;
    float right1 = (lx1 < rx1) ? rx1 : lx1;
    float minL = (left < left1) ? left : left1;
    float maxR = (right > right1) ? right : right1;
    DrawRectangle((int)minL, (int)top, (int)(maxR - minL), (int)(bot - top) + 1, col);
  }

  // Draw track edges (yellow lines)
  for (int i = 0; i < valid - 1; i++) {
    DrawLineEx((Vector2){lxArr[i], lyArr[i]}, (Vector2){lxArr[i+1], lyArr[i+1]}, 2.0f, YELLOW);
    DrawLineEx((Vector2){rxArr[i], ryArr[i]}, (Vector2){rxArr[i+1], ryArr[i+1]}, 2.0f, YELLOW);
  }

  // Draw center dashed line
  for (int i = 0; i < valid - 1; i += 2) {
    float clx = (lxArr[i] + rxArr[i]) * 0.5f;
    float cly = (lyArr[i] + ryArr[i]) * 0.5f;
    float crx = (lxArr[i+1] + rxArr[i+1]) * 0.5f;
    float cry = (lyArr[i+1] + ryArr[i+1]) * 0.5f;
    DrawLineEx((Vector2){clx, cly}, (Vector2){crx, cry}, 1.5f, (Color){200, 200, 200, 180});
  }
}

// === Scenery ===

static void DrawScenery(const Game *game, float steerOff) {
  const Track *track = &game->track;
  int N = track->pointCount;

  for (int i = 0; i < N; i++) {
    int nidx = (i + 1) % N;
    Vector2 p = track->points[i];
    Vector2 np = track->points[nidx];

    float dx = np.x - p.x;
    float dy = np.y - p.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) continue;
    float px = -dy / len;
    float py = dx / len;

    float treeOffset = track->halfWidth * 2.5f;

    // Left side tree
    Vector2 tl = { p.x + px * treeOffset, p.y + py * treeOffset };
    float llx, lly;
    WorldToLocal(game, tl, &llx, &lly);
    float lsx, lsy, lsc;
    if (Project(llx, lly, steerOff, &lsx, &lsy, &lsc)) {
      float treeR = 15.0f * lsc;
      if (treeR > 1.0f && treeR < 200.0f) {
        DrawRectangle((int)lsx - (int)(2 * lsc), (int)lsy, (int)(4 * lsc), (int)(10 * lsc), (Color){80, 50, 20, 255});
        DrawCircle((int)lsx, (int)(lsy - 5 * lsc), (int)treeR, (Color){30, 120, 30, 255});
      }
    }

    // Right side tree (every other point for variety)
    if (i % 2 == 0) {
      Vector2 tr = { p.x - px * treeOffset * 1.2f, p.y - py * treeOffset * 1.2f };
      float rlx, rly;
      WorldToLocal(game, tr, &rlx, &rly);
      float rsx, rsy, rsc;
      if (Project(rlx, rly, steerOff, &rsx, &rsy, &rsc)) {
        float treeR = 12.0f * rsc;
        if (treeR > 1.0f && treeR < 200.0f) {
          DrawRectangle((int)rsx - (int)(2 * rsc), (int)rsy, (int)(4 * rsc), (int)(8 * rsc), (Color){80, 50, 20, 255});
          DrawCircle((int)rsx, (int)(rsy - 4 * rsc), (int)treeR, (Color){40, 140, 40, 255});
        }
      }
    }
  }
}

// === Spectators ===

static void DrawSpectators(const Game *game, float steerOff) {
  const Track *track = &game->track;
  int N = track->pointCount;
  int count = game->spectatorCount;
  if (count <= 0) return;

  for (int i = 0; i < count; i++) {
    int trackIdx = (i * N) / count;
    int nidx = (trackIdx + 1) % N;
    Vector2 p = track->points[trackIdx];
    Vector2 np = track->points[nidx];

    float dx = np.x - p.x;
    float dy = np.y - p.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) continue;
    float px = -dy / len;
    float py = dx / len;

    float side = (i % 2 == 0) ? 1.0f : -1.0f;
    float offset = track->halfWidth * 1.5f;

    Vector2 sp = { p.x + px * offset * side, p.y + py * offset * side };
    float lx, ly;
    WorldToLocal(game, sp, &lx, &ly);
    float sx, sy, scale;
    if (!Project(lx, ly, steerOff, &sx, &sy, &scale)) continue;

    float bodyW = 6.0f * scale;
    float bodyH = 12.0f * scale;
    if (bodyW < 1.0f) continue;
    if (bodyW > 40.0f) bodyW = 40.0f;

    int r = 100 + (i * 37) % 155;
    int g = 80 + (i * 53) % 175;
    int b = 80 + (i * 71) % 175;
    Color bodyCol = { (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };

    // Jump animation: each spectator bounces at a slightly different phase
    float time = GetTime();
    float phase = (float)i * 0.7f;
    float jump = sinf(time * 6.0f + phase) * 0.5f + 0.5f; // 0..1
    float jumpOffset = jump * 4.0f * scale;

    float headR = 3.0f * scale;
    DrawCircle((int)sx, (int)(sy - bodyH * 0.8f - jumpOffset), (int)headR, (Color){220, 180, 150, 255});
    DrawRectangle((int)(sx - bodyW / 2), (int)(sy - bodyH * 0.6f - jumpOffset), (int)bodyW, (int)(bodyH * 0.6f), bodyCol);
  }
}

// === Boost coins ===

static void DrawBoostCoins(const Game *game, float steerOff) {
  const Track *track = &game->track;
  int N = track->pointCount;

  for (int i = 0; i < game->boostCoinCount; i++) {
    if (game->boostCollected[i]) continue;

    Vector2 coinPos = track->points[game->boostCoins[i] % N];
    float lx, ly;
    WorldToLocal(game, coinPos, &lx, &ly);
    float sx, sy, scale;
    if (!Project(lx, ly, steerOff, &sx, &sy, &scale)) continue;

    float r = 10.0f * scale;
    if (r < 2.0f) continue;
    if (r > 60.0f) r = 60.0f;

    DrawCircle((int)sx, (int)sy, (int)(r * 1.3f), (Color){255, 200, 0, 80});
    DrawCircle((int)sx, (int)sy, (int)r, GOLD);
    DrawCircleLines((int)sx, (int)sy, (int)r, (Color){255, 255, 100, 255});
    DrawLine((int)(sx - r * 0.4f), (int)sy, (int)(sx + r * 0.4f), (int)sy, (Color){255, 255, 200, 255});
    DrawLine((int)sx, (int)(sy - r * 0.4f), (int)sx, (int)(sy + r * 0.4f), (Color){255, 255, 200, 255});
  }
}

// === Hypersonic line ===

static void DrawHypersonicLine(const Game *game, float steerOff) {
  const Track *track = &game->track;
  int N = track->pointCount;
  int idx = game->hypersonicIdx % N;
  int nidx = (idx + 1) % N;

  Vector2 p = track->points[idx];
  Vector2 np = track->points[nidx];

  float dx = np.x - p.x;
  float dy = np.y - p.y;
  float len = sqrtf(dx * dx + dy * dy);
  if (len < 0.001f) return;
  float px = -dy / len;
  float py = dx / len;

  Vector2 left = { p.x + px * track->halfWidth, p.y + py * track->halfWidth };
  Vector2 right = { p.x - px * track->halfWidth, p.y - py * track->halfWidth };

  float llx, lly, rlx, rly;
  WorldToLocal(game, left, &llx, &lly);
  WorldToLocal(game, right, &rlx, &rly);

  float lsx, lsy, lsc, rsx, rsy, rsc;
  if (!Project(llx, lly, steerOff, &lsx, &lsy, &lsc)) return;
  if (!Project(rlx, rly, steerOff, &rsx, &rsy, &rsc)) return;

  float thickness = 4.0f * lsc;
  if (thickness < 1.0f) thickness = 1.0f;
  if (thickness > 12.0f) thickness = 12.0f;

  DrawLineEx((Vector2){lsx, lsy}, (Vector2){rsx, rsy}, thickness * 2.5f, (Color){255, 0, 255, 60});
  DrawLineEx((Vector2){lsx, lsy}, (Vector2){rsx, rsy}, thickness, (Color){255, 0, 255, 255});
  DrawLineEx((Vector2){lsx, lsy}, (Vector2){rsx, rsy}, thickness * 0.4f, (Color){255, 200, 255, 255});
}

// === Pit zone indicator ===

static void DrawPitZone(const Game *game, float steerOff) {
  const Track *track = &game->track;
  int N = track->pointCount;
  int idx = game->pitZoneIdx % N;
  int nidx = (idx + 1) % N;

  Vector2 p = track->points[idx];
  Vector2 np = track->points[nidx];

  float dx = np.x - p.x;
  float dy = np.y - p.y;
  float len = sqrtf(dx * dx + dy * dy);
  if (len < 0.001f) return;
  float px = -dy / len;
  float py = dx / len;

  Vector2 left = { p.x + px * track->halfWidth, p.y + py * track->halfWidth };
  Vector2 right = { p.x - px * track->halfWidth, p.y - py * track->halfWidth };

  float llx, lly, rlx, rly;
  WorldToLocal(game, left, &llx, &lly);
  WorldToLocal(game, right, &rlx, &rly);

  float lsx, lsy, lsc, rsx, rsy, rsc;
  if (!Project(llx, lly, steerOff, &lsx, &lsy, &lsc)) return;
  if (!Project(rlx, rly, steerOff, &rsx, &rsy, &rsc)) return;

  float thickness = 6.0f * lsc;
  if (thickness < 2.0f) thickness = 2.0f;
  if (thickness > 16.0f) thickness = 16.0f;

  DrawLineEx((Vector2){lsx, lsy}, (Vector2){rsx, rsy}, thickness, (Color){255, 140, 0, 200});

  float midX = (lsx + rsx) * 0.5f;
  float midY = (lsy + rsy) * 0.5f;
  float fontSize = 14.0f * lsc;
  if (fontSize < 8.0f) fontSize = 8.0f;
  if (fontSize > 30.0f) fontSize = 30.0f;
  int fs = (int)fontSize;
  int tw = MeasureText("PIT", fs);
  DrawText("PIT", (int)(midX - tw / 2), (int)(midY - fs * 1.5f), fs, (Color){255, 140, 0, 255});
}

// === AI Cars in windshield ===

static void DrawAICarsAhead(const Game *game, float steerOff) {
  for (int i = 0; i < game->aiCarCount; i++) {
    const AICar *ai = &game->aiCars[i];
    float lx, ly;
    WorldToLocal(game, ai->position, &lx, &ly);

    if (ly < 5.0f) continue;

    float sx, sy, scale;
    if (!Project(lx, ly, steerOff, &sx, &sy, &scale)) continue;

    float cw = 44.0f * scale;
    float ch = 24.0f * scale;
    if (cw < 2.0f) continue;
    if (cw > 400.0f) cw = 400.0f;

    int x = (int)(sx - cw / 2);
    int y = (int)(sy - ch / 2);
    int w = (int)cw;
    int h = (int)ch;

    DrawRectangle(x, y, w, h, ai->color);
    Color windCol = Fade(ai->color, 0.4f);
    DrawRectangle(x + (int)(w * 0.3f), y + (int)(h * 0.15f), (int)(w * 0.4f), (int)(h * 0.35f), windCol);

    Color wheelCol = BLACK;
    int ww = (int)(w * 0.15f);
    int wh = (int)(h * 0.25f);
    DrawRectangle(x, y + (int)(h * 0.1f), ww, wh, wheelCol);
    DrawRectangle(x + w - ww, y + (int)(h * 0.1f), ww, wh, wheelCol);
    DrawRectangle(x, y + (int)(h * 0.65f), ww, wh, wheelCol);
    DrawRectangle(x + w - ww, y + (int)(h * 0.65f), ww, wh, wheelCol);
  }
}

// === Rear mirror ===

static void DrawRearMirror(const Game *game) {
  DrawRectangle(MIRROR_X, MIRROR_Y, MIRROR_W, MIRROR_H, (Color){30, 30, 35, 255});
  DrawRectangleLines(MIRROR_X, MIRROR_Y, MIRROR_W, MIRROR_H, (Color){100, 100, 100, 255});

  int miX = MIRROR_X + 4;
  int miY = MIRROR_Y + 4;
  int miW = MIRROR_W - 8;
  int miH = MIRROR_H - 8;

  // Sky in mirror
  DrawRectangle(miX, miY, miW, miH / 3, (Color){135, 206, 235, 255});
  // Ground in mirror
  DrawRectangle(miX, miY + miH / 3, miW, miH - miH / 3, (Color){50, 80, 50, 255});

  int mirrorHorizon = miY + miH / 3;
  int mirrorBottom = miY + miH;
  int mirrorCenterX = MIRROR_X + MIRROR_W / 2;

  // Road in mirror (trapezoid)
  int roadWBottom = (int)(miW * 0.7f);
  int roadWTop = (int)(miW * 0.15f);
  int roadH = mirrorBottom - mirrorHorizon;
  DrawRectangle(mirrorCenterX - roadWBottom / 2, mirrorHorizon, roadWBottom, roadH, (Color){60, 60, 60, 255});
  DrawRectangle(mirrorCenterX - roadWTop / 2, mirrorHorizon, roadWTop, roadH / 2, (Color){55, 55, 55, 255});

  // Draw AI cars behind
  for (int i = 0; i < game->aiCarCount; i++) {
    const AICar *ai = &game->aiCars[i];
    float lx, ly;
    WorldToLocal(game, ai->position, &lx, &ly);

    if (ly > -5.0f) continue;

    float dist = -ly;
    if (dist > 500.0f) continue;

    float mScale = 80.0f / dist;
    float msx = mirrorCenterX + lx * mScale;
    float msy = mirrorHorizon + (mirrorBottom - mirrorHorizon) * (10.0f / dist);

    float cw = 44.0f * mScale;
    float ch = 24.0f * mScale;
    if (cw < 2.0f) continue;
    if (cw > 80.0f) cw = 80.0f;

    int x = (int)(msx - cw / 2);
    int y = (int)(msy - ch / 2);
    int w = (int)cw;
    int h = (int)ch;

    if (x + w < miX || x > miX + miW) continue;
    if (y + h < miY || y > miY + miH) continue;

    DrawRectangle(x, y, w, h, ai->color);
    Color windCol = Fade(ai->color, 0.4f);
    DrawRectangle(x + (int)(w * 0.3f), y + (int)(h * 0.15f), (int)(w * 0.4f), (int)(h * 0.35f), windCol);
  }

  DrawText("REAR", MIRROR_X + MIRROR_W / 2 - 15, MIRROR_Y + MIRROR_H - 14, 10, (Color){200, 200, 200, 128});
}

// === Dashboard ===

static void DrawDashboard(const Game *game) {
  DrawRectangle(0, DASH_TOP, 1280, DASH_BOTTOM - DASH_TOP, (Color){25, 25, 30, 255});
  DrawLine(0, DASH_TOP, 1280, DASH_TOP, (Color){60, 60, 65, 255});

  char buf[64];

  // --- Speedometer (left) ---
  int spX = 150;
  int spY = 610;
  int spR = 80;

  DrawCircleSector((Vector2){spX, spY}, (float)spR, 200.0f, 340.0f, 30, (Color){40, 40, 45, 255});
  DrawCircleSector((Vector2){spX, spY}, (float)(spR - 8), 200.0f, 340.0f, 30, (Color){30, 30, 35, 255});

  for (int i = 0; i <= 10; i++) {
    float angle = 200.0f + 140.0f * i / 10.0f;
    float rad = angle * PI / 180.0f;
    float x1 = spX + cosf(rad) * (spR - 12);
    float y1 = spY + sinf(rad) * (spR - 12);
    float x2 = spX + cosf(rad) * (spR - 4);
    float y2 = spY + sinf(rad) * (spR - 4);
    Color tickCol = (i >= 8) ? RED : WHITE;
    DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 2.0f, tickCol);
  }

  float speedNorm = game->car.speed / 400.0f;
  if (speedNorm < 0.0f) speedNorm = 0.0f;
  if (speedNorm > 1.0f) speedNorm = 1.0f;
  float needleAngle = 200.0f + 140.0f * speedNorm;
  float nRad = needleAngle * PI / 180.0f;
  float nx = spX + cosf(nRad) * (spR - 15);
  float ny = spY + sinf(nRad) * (spR - 15);
  DrawLineEx((Vector2){spX, spY}, (Vector2){nx, ny}, 3.0f, RED);
  DrawCircle(spX, spY, 5, RED);

  snprintf(buf, sizeof(buf), "%.0f", game->car.speed);
  int tw = MeasureText(buf, 28);
  DrawText(buf, spX - tw / 2, spY + 15, 28, WHITE);
  DrawText("km/h", spX - 20, spY + 45, 14, LIGHTGRAY);

  // --- RPM bar ---
  int rpmX = 300;
  int rpmY = 560;
  int rpmW = 120;
  int rpmH = 16;

  DrawRectangle(rpmX, rpmY, rpmW, rpmH, (Color){40, 40, 45, 255});
  DrawRectangleLines(rpmX, rpmY, rpmW, rpmH, GRAY);

  int rpmFill = (int)(rpmW * speedNorm);
  Color rpmCol = (speedNorm > 0.8f) ? RED : (speedNorm > 0.6f) ? ORANGE : LIME;
  DrawRectangle(rpmX, rpmY, rpmFill, rpmH, rpmCol);
  DrawText("RPM", rpmX, rpmY - 18, 14, LIGHTGRAY);

  // --- Gear indicator ---
  int gear = 1;
  if (game->car.speed > 300.0f) gear = 6;
  else if (game->car.speed > 240.0f) gear = 5;
  else if (game->car.speed > 180.0f) gear = 4;
  else if (game->car.speed > 120.0f) gear = 3;
  else if (game->car.speed > 60.0f) gear = 2;
  if (game->car.speed < 5.0f) gear = 0;

  snprintf(buf, sizeof(buf), "G%d", gear);
  DrawText(buf, 470, 570, 36, (gear == 0) ? GRAY : WHITE);
  DrawText("GEAR", 465, 610, 14, LIGHTGRAY);

  // --- Lap info ---
  snprintf(buf, sizeof(buf), "LAP %d/%d", game->lap + 1, game->totalLaps);
  DrawText(buf, 580, 555, 24, WHITE);

  snprintf(buf, sizeof(buf), "TIME  %.2f", game->raceTime);
  DrawText(buf, 580, 590, 18, LIGHTGRAY);

  snprintf(buf, sizeof(buf), "LAP   %.2f", game->lapTime);
  DrawText(buf, 580, 620, 18, LIGHTGRAY);

  if (game->bestLap > 0.0f) {
    snprintf(buf, sizeof(buf), "BEST  %.2f", game->bestLap);
    DrawText(buf, 580, 650, 18, GOLD);
  }

  // --- Off-track warning ---
  if (!TrackIsOnTrack(&game->track, game->car.position)) {
    DrawText("OFF TRACK!", 750, 570, 28, RED);
  }

  // --- Fuel gauge ---
  int fgX = 580;
  int fgY = 660;
  int fgW = 150;
  int fgH = 14;
  DrawRectangle(fgX, fgY, fgW, fgH, (Color){40, 40, 45, 255});
  DrawRectangleLines(fgX, fgY, fgW, fgH, GRAY);
  int fuelFill = (int)(fgW * game->fuel / 100.0f);
  Color fuelCol = (game->fuel < 20.0f) ? RED : (game->fuel < 40.0f) ? ORANGE : LIME;
  DrawRectangle(fgX, fgY, fuelFill, fgH, fuelCol);
  DrawText("FUEL", fgX, fgY - 16, 12, LIGHTGRAY);

  // --- Tire pressure (4 small bars) ---
  const char *tireLabels[4] = {"FL", "FR", "RL", "RR"};
  for (int i = 0; i < 4; i++) {
    int tx = 750 + i * 35;
    int ty = 660;
    int tw = 28;
    int th = 14;
    DrawRectangle(tx, ty, tw, th, (Color){40, 40, 45, 255});
    int tFill = (int)(tw * game->tirePressure[i] / 100.0f);
    Color tCol = (game->tirePressure[i] < 30.0f) ? RED : (game->tirePressure[i] < 60.0f) ? ORANGE : LIME;
    DrawRectangle(tx, ty, tFill, th, tCol);
    DrawText(tireLabels[i], tx + 4, ty - 14, 10, LIGHTGRAY);
  }

  // --- Warning lights ---
  int wlX = 750;
  int wlY = 690;
  if (game->warningLights & WARN_OVERHEAT) { DrawText("TEMP", wlX, wlY, 12, RED); wlX += 45; }
  if (game->warningLights & WARN_OIL) { DrawText("OIL", wlX, wlY, 12, ORANGE); wlX += 40; }
  if (game->warningLights & WARN_SPARK) { DrawText("SPK", wlX, wlY, 12, YELLOW); wlX += 40; }
  if (game->warningLights & WARN_FUEL) { DrawText("FUEL", wlX, wlY, 12, RED); }

  // --- Boost indicators ---
  if (game->boostTimer > 0.0f) {
    DrawText("TURBO!", 580, 530, 24, (Color){0, 255, 255, 255});
  }
  if (game->hypersonicTimer > 0.0f) {
    DrawText("HYPersonic!", 580, 505, 24, (Color){255, 0, 255, 255});
  }

  // --- Pit stop overlay ---
  if (game->inPitStop) {
    DrawRectangle(0, 0, 1280, 720, (Color){0, 0, 0, 180});
    DrawText("PIT STOP", 540, 150, 60, GOLD);
    DrawText("Fixing tires...", 540, 250, 30, WHITE);
    snprintf(buf, sizeof(buf), "%.1fs", game->pitStopTimer);
    DrawText(buf, 600, 300, 40, LIME);

    // Engine tuning
    DrawText("ENGINE TUNING [LEFT/RIGHT]", 440, 380, 24, WHITE);
    int tuneW = 300;
    int tuneX = 640 - tuneW / 2;
    int tuneY = 420;
    DrawRectangle(tuneX, tuneY, tuneW, 20, (Color){40, 40, 45, 255});
    DrawRectangleLines(tuneX, tuneY, tuneW, 20, GRAY);
    int tuneFill = (int)(tuneW * game->engineTuning);
    DrawRectangle(tuneX, tuneY, tuneFill, 20, (Color){0, 150, 255, 255});
    snprintf(buf, sizeof(buf), "%.0f%%", game->engineTuning * 100.0f);
    DrawText(buf, 620, 450, 20, WHITE);
    DrawText("Low = safe  |  High = power (risky)", 470, 480, 16, LIGHTGRAY);

  }

  // --- Minimap (right) ---
  int mmX = 1060;
  int mmY = 530;
  int mmW = 200;
  int mmH = 170;
  float mmScale = 0.08f;

  DrawRectangle(mmX, mmY, mmW, mmH, (Color){15, 15, 20, 255});
  DrawRectangleLines(mmX, mmY, mmW, mmH, (Color){80, 80, 85, 255});

  for (int i = 0; i < game->track.pointCount; i++) {
    int next = (i + 1) % game->track.pointCount;
    Vector2 a = game->track.points[i];
    Vector2 b = game->track.points[next];
    Vector2 ma = { mmX + a.x * mmScale, mmY + a.y * mmScale };
    Vector2 mb = { mmX + b.x * mmScale, mmY + b.y * mmScale };
    DrawLineEx(ma, mb, 2.0f, DARKGRAY);
  }

  for (int i = 0; i < game->aiCarCount; i++) {
    Vector2 aiMM = { mmX + game->aiCars[i].position.x * mmScale,
                    mmY + game->aiCars[i].position.y * mmScale };
    DrawCircleV(aiMM, 3.0f, game->aiCars[i].color);
  }

  Vector2 carMM = { mmX + game->car.position.x * mmScale,
                   mmY + game->car.position.y * mmScale };
  DrawCircleV(carMM, 4.0f, RED);
}

// === Kangaroo ===

static void DrawKangaroo(const Game *game, float steerOff) {
  if (!game->kangarooActive) return;

  float t = game->kangarooJumpT;
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;

  // Kangaroo position: arc from start to landing
  // We stored the landing spot in kangarooPos; the start was offset
  // For visual: interpolate with a parabolic arc
  Vector2 kp = game->kangarooPos;
  float lx, ly;
  WorldToLocal(game, kp, &lx, &ly);
  float sx, sy, scale;
  if (!Project(lx, ly, steerOff, &sx, &sy, &scale)) return;

  // Jump arc: parabolic height
  float arcHeight = sinf(t * PI) * 30.0f * scale;
  float ky = sy - arcHeight;

  float bodyW = 20.0f * scale;
  float bodyH = 30.0f * scale;
  if (bodyW < 2.0f) return;
  if (bodyW > 120.0f) bodyW = 120.0f;

  Color kangCol = { 160, 120, 60, 255 };
  Color kangDark = { 120, 80, 40, 255 };

  // Body
  DrawEllipse((int)sx, (int)ky, (int)(bodyW * 0.6f), (int)(bodyH * 0.5f), kangCol);
  // Head
  DrawCircle((int)(sx + bodyW * 0.3f), (int)(ky - bodyH * 0.4f), (int)(bodyW * 0.25f), kangCol);
  // Ear
  DrawCircle((int)(sx + bodyW * 0.4f), (int)(ky - bodyH * 0.55f), (int)(bodyW * 0.1f), kangDark);
  // Tail
  DrawLineEx((Vector2){sx - bodyW * 0.5f, ky},
             (Vector2){sx - bodyW * 0.8f, ky + bodyH * 0.3f},
             3.0f * scale, kangDark);
  // Legs
  float legSpread = sinf(t * PI) * bodyW * 0.3f;
  DrawLineEx((Vector2){sx - bodyW * 0.2f, ky + bodyH * 0.3f},
             (Vector2){sx - bodyW * 0.2f - legSpread, ky + bodyH * 0.6f},
             2.0f * scale, kangDark);
  DrawLineEx((Vector2){sx + bodyW * 0.2f, ky + bodyH * 0.3f},
             (Vector2){sx + bodyW * 0.2f + legSpread, ky + bodyH * 0.6f},
             2.0f * scale, kangDark);
}

// === Windshield broken overlay ===

static void DrawWindshieldBroken(const Game *game) {
  if (!game->windshieldBroken) return;

  // Cracked glass effect: radial lines from center
  int cx = 640, cy = 250;
  for (int i = 0; i < 12; i++) {
    float angle = (float)i * (PI / 6.0f) + 0.3f;
    float len = 150.0f + (float)((i * 17) % 100);
    float ex = (float)cx + cosf(angle) * len;
    float ey = (float)cy + sinf(angle) * len;
    DrawLineEx((Vector2){(float)cx, (float)cy}, (Vector2){ex, ey}, 2.0f, (Color){200, 220, 255, 180});
  }
  // Inner cracks
  for (int i = 0; i < 8; i++) {
    float angle = (float)i * (PI / 4.0f) + 0.7f;
    float len = 60.0f + (float)((i * 23) % 50);
    float ex = (float)cx + cosf(angle) * len;
    float ey = (float)cy + sinf(angle) * len;
    DrawLineEx((Vector2){(float)cx, (float)cy}, (Vector2){ex, ey}, 1.0f, (Color){255, 255, 255, 120});
  }
  // Center impact point
  DrawCircle(cx, cy, 8, (Color){255, 255, 255, 200});
}

// === Main first-person draw ===

void FirstPersonDraw(const Game *game) {
  float steerOff = game->car.steering * 20.0f;

  // Sky
  DrawRectangle(0, WS_TOP, 1280, HORIZON_Y - WS_TOP, (Color){135, 206, 235, 255});

  // Ground (grass)
  DrawRectangle(0, HORIZON_Y, 1280, WS_BOTTOM - HORIZON_Y, (Color){50, 120, 50, 255});

  // Road
  DrawRoad(game, steerOff);

  // Scenery
  DrawScenery(game, steerOff);

  // Spectators
  DrawSpectators(game, steerOff);

  // Boost coins
  DrawBoostCoins(game, steerOff);

  // Hypersonic line
  DrawHypersonicLine(game, steerOff);

  // Pit zone
  DrawPitZone(game, steerOff);

  // AI cars ahead
  DrawAICarsAhead(game, steerOff);

  // Kangaroo
  DrawKangaroo(game, steerOff);

  // Rear mirror
  DrawRearMirror(game);

  // Windshield broken overlay
  DrawWindshieldBroken(game);

  // Dashboard
  DrawDashboard(game);

  // Windshield frame
  DrawRectangleLines(0, 0, 1280, WS_BOTTOM, (Color){40, 40, 45, 128});
}