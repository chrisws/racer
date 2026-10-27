#include "track.h"
#include <math.h>

// Mount Panorama-inspired circuit (Bathurst 1000, NSW Australia)
// Key sections: Main Straight, The Loop, The Dip, The Pylon,
// The Cutting Room, The Hairpin, The Esses, The Chicanes
static const Vector2 controlPoints[] = {
  { 200,  900 },   //  0: start/finish (bottom-left)
  { 600,  900 },   //  1: main straight
  { 1000, 900 },   //  2: main straight
  { 1350, 880 },   //  3: approach to The Loop
  { 1500, 750 },   //  4: The Loop (tight left)
  { 1400, 600 },   //  5: The Loop exit
  { 1250, 500 },   //  6: The Dip (sweeping right)
  { 1350, 350 },   //  7: The Pylon (tight right)
  { 1200, 200 },   //  8: The Cutting Room entry
  { 1050, 250 },   //  9: The Cutting Room chicane
  { 900,  150 },   // 10: The Hairpin (tight left)
  { 750,  200 },   // 11: The Hairpin exit
  { 650,  300 },   // 12: The Esses entry
  { 550,  250 },   // 13: The Esses mid
  { 450,  350 },   // 14: The Esses exit
  { 350,  300 },   // 15: The Chicanes entry
  { 250,  400 },   // 16: The Chicanes mid
  { 200,  550 },   // 17: The Chicanes exit
  { 150,  700 },   // 18: back curve
  { 200,  850 },   // 19: approach to start
};

#define CONTROL_POINT_COUNT 20

void TrackInit(Track *track) {
  track->pointCount = CONTROL_POINT_COUNT;
  for (int i = 0; i < CONTROL_POINT_COUNT; i++) {
    track->points[i] = controlPoints[i];
  }
  track->halfWidth = 60.0f;
}

// Catmull-Rom spline interpolation for smooth track curves
static Vector2 CatmullRom(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float t) {
  float t2 = t * t;
  float t3 = t2 * t;
  Vector2 result;
  result.x = 0.5f * ((2.0f * p1.x) +
    (-p0.x + p2.x) * t +
    (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
    (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
  result.y = 0.5f * ((2.0f * p1.y) +
    (-p0.y + p2.y) * t +
    (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
    (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
  return result;
}

void TrackSmooth(Track *track, int subdivisions) {
  int N = track->pointCount;
  if (N < 4) return;

  Vector2 smoothed[MAX_TRACK_POINTS];
  int count = 0;

  for (int i = 0; i < N; i++) {
    Vector2 p0 = track->points[(i - 1 + N) % N];
    Vector2 p1 = track->points[i];
    Vector2 p2 = track->points[(i + 1) % N];
    Vector2 p3 = track->points[(i + 2) % N];

    for (int j = 0; j < subdivisions; j++) {
      float t = (float)j / (float)subdivisions;
      smoothed[count++] = CatmullRom(p0, p1, p2, p3, t);
    }
  }

  for (int i = 0; i < count; i++) {
    track->points[i] = smoothed[i];
  }
  track->pointCount = count;
}

void TrackDraw(const Track *track) {
  Color trackColor = DARKGRAY;
  Color shoulderColor = (Color){ 40, 40, 40, 255 };
  Color edgeColor = YELLOW;

  int N = track->pointCount;

  // Compute left and right edge points using averaged normals for smoothness
  Vector2 leftEdge[MAX_TRACK_POINTS];
  Vector2 rightEdge[MAX_TRACK_POINTS];
  Vector2 shoulderL[MAX_TRACK_POINTS];
  Vector2 shoulderR[MAX_TRACK_POINTS];

  for (int i = 0; i < N; i++) {
    int prev = (i - 1 + N) % N;
    int next = (i + 1) % N;
    Vector2 a = track->points[i];

    // Average of incoming and outgoing directions for a smooth normal
    float dx1 = a.x - track->points[prev].x;
    float dy1 = a.y - track->points[prev].y;
    float dx2 = track->points[next].x - a.x;
    float dy2 = track->points[next].y - a.y;
    float dx = dx1 + dx2;
    float dy = dy1 + dy2;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) {
      leftEdge[i] = a;
      rightEdge[i] = a;
      shoulderL[i] = a;
      shoulderR[i] = a;
      continue;
    }
    float px = -dy / len;
    float py = dx / len;

    leftEdge[i].x = a.x + px * track->halfWidth;
    leftEdge[i].y = a.y + py * track->halfWidth;
    rightEdge[i].x = a.x - px * track->halfWidth;
    rightEdge[i].y = a.y - py * track->halfWidth;
    shoulderL[i].x = a.x + px * track->halfWidth * 3.0f;
    shoulderL[i].y = a.y + py * track->halfWidth * 3.0f;
    shoulderR[i].x = a.x - px * track->halfWidth * 3.0f;
    shoulderR[i].y = a.y - py * track->halfWidth * 3.0f;
  }

  // Draw shoulder polygon (wider, darker)
  {
    Vector2 shoulderPoly[MAX_TRACK_POINTS * 2];
    for (int i = 0; i < N; i++) shoulderPoly[i] = shoulderL[i];
    for (int i = N - 1; i >= 0; i--) shoulderPoly[N + i] = shoulderR[i];
    DrawTriangleStrip(shoulderPoly, N * 2 - 2, shoulderColor);
  }

  // Draw track surface as a filled polygon (no gaps at joints)
  {
    Vector2 trackPoly[MAX_TRACK_POINTS * 2];
    for (int i = 0; i < N; i++) trackPoly[i] = leftEdge[i];
    for (int i = N - 1; i >= 0; i--) trackPoly[N + i] = rightEdge[i];
    DrawTriangleStrip(trackPoly, N * 2 - 2, trackColor);
  }

  // Draw track edges as continuous lines
  for (int i = 0; i < N; i++) {
    int next = (i + 1) % N;
    DrawLineEx(leftEdge[i], leftEdge[next], 3.0f, edgeColor);
    DrawLineEx(rightEdge[i], rightEdge[next], 3.0f, edgeColor);
  }

  // Draw start/finish line
  Vector2 start = track->points[0];
  Vector2 next = track->points[1];
  float dx = next.x - start.x;
  float dy = next.y - start.y;
  float len = sqrtf(dx * dx + dy * dy);
  if (len > 0.001f) {
    float px = -dy / len;
    float py = dx / len;
    Vector2 sL = { start.x + px * track->halfWidth, start.y + py * track->halfWidth };
    Vector2 sR = { start.x - px * track->halfWidth, start.y - py * track->halfWidth };
    DrawLineEx(sL, sR, 4.0f, WHITE);
  }
}

// Distance from point to line segment
static float PointToSegmentDist(Vector2 p, Vector2 a, Vector2 b) {
  float abx = b.x - a.x;
  float aby = b.y - a.y;
  float apx = p.x - a.x;
  float apy = p.y - a.y;
  float abLen2 = abx * abx + aby * aby;
  if (abLen2 < 0.001f) return sqrtf(apx * apx + apy * apy);
  float t = (apx * abx + apy * aby) / abLen2;
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  float cx = a.x + t * abx;
  float cy = a.y + t * aby;
  float dx = p.x - cx;
  float dy = p.y - cy;
  return sqrtf(dx * dx + dy * dy);
}

float TrackGetNearestDistance(const Track *track, Vector2 pos) {
  float minDist = 1e9f;
  for (int i = 0; i < track->pointCount; i++) {
    int next = (i + 1) % track->pointCount;
    float d = PointToSegmentDist(pos, track->points[i], track->points[next]);
    if (d < minDist) minDist = d;
  }
  return minDist;
}

Vector2 TrackGetNearestPoint(const Track *track, Vector2 pos) {
  float minDist = 1e9f;
  Vector2 nearest = track->points[0];
  for (int i = 0; i < track->pointCount; i++) {
    int next = (i + 1) % track->pointCount;
    Vector2 a = track->points[i];
    Vector2 b = track->points[next];
    float abx = b.x - a.x;
    float aby = b.y - a.y;
    float abLen2 = abx * abx + aby * aby;
    if (abLen2 < 0.001f) continue;
    float t = ((pos.x - a.x) * abx + (pos.y - a.y) * aby) / abLen2;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    Vector2 proj = { a.x + t * abx, a.y + t * aby };
    float dx = pos.x - proj.x;
    float dy = pos.y - proj.y;
    float d = sqrtf(dx * dx + dy * dy);
    if (d < minDist) {
      minDist = d;
      nearest = proj;
    }
  }
  return nearest;
}

int TrackIsOnTrack(const Track *track, Vector2 pos) {
  return TrackGetNearestDistance(track, pos) <= track->halfWidth;
}

int TrackGetNearestSegment(const Track *track, Vector2 pos) {
  int nearest = 0;
  float minDist = 1e9f;
  for (int i = 0; i < track->pointCount; i++) {
    int next = (i + 1) % track->pointCount;
    float d = PointToSegmentDist(pos, track->points[i], track->points[next]);
    if (d < minDist) {
      minDist = d;
      nearest = i;
    }
  }
  return nearest;
}