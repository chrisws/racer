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
  *lx = dx * c - dy * s;
  *ly = dx * s + dy * c;
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

  // Draw road surface as horizontal strips
  for (int i = 0; i < valid; i++) {
    float screenY = lyArr[i];
    float lx = lxArr[i];
    float rx = rxArr[i];
    Color col = (i % 4 < 2) ? (Color){65, 65, 65, 255} : (Color){58, 58, 58, 255};
    DrawLineEx((Vector2){lx, screenY}, (Vector2){rx, screenY}, 2.5f, col);
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

// === Main first-person draw ===

void FirstPersonDraw(const Game *game) {
  float steerOff = -game->car.steering * 20.0f;

  // Sky
  DrawRectangle(0, WS_TOP, 1280, HORIZON_Y - WS_TOP, (Color){135, 206, 235, 255});

  // Ground (grass)
  DrawRectangle(0, HORIZON_Y, 1280, WS_BOTTOM - HORIZON_Y, (Color){50, 120, 50, 255});

  // Road
  DrawRoad(game, steerOff);

  // Scenery
  DrawScenery(game, steerOff);

  // AI cars ahead
  DrawAICarsAhead(game, steerOff);

  // Rear mirror
  DrawRearMirror(game);

  // Dashboard
  DrawDashboard(game);

  // Windshield frame
  DrawRectangleLines(0, 0, 1280, WS_BOTTOM, (Color){40, 40, 45, 128});
}