#include "track.h"
#include <math.h>

// Mount Panorama-inspired circuit (Bathurst 1000, NSW Australia)
// Key sections: Main Straight, The Loop, The Dip, The Pylon,
// The Cutting Room, The Hairpin, The Esses, The Chicanes
static const Vector2 defaultTrackPoints[] = {
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

void TrackInit(Track *track) {
  track->pointCount = (int)(sizeof(defaultTrackPoints) / sizeof(defaultTrackPoints[0]));
  for (int i = 0; i < track->pointCount; i++) {
    track->points[i] = defaultTrackPoints[i];
  }
  track->halfWidth = 60.0f;
}

void TrackDraw(const Track *track) {
  Color trackColor = DARKGRAY;
  Color shoulderColor = (Color){ 40, 40, 40, 255 };
  Color edgeColor = YELLOW;

  // Draw shoulder/runoff (wider, darker) for visual context
  for (int i = 0; i < track->pointCount; i++) {
    int next = (i + 1) % track->pointCount;
    DrawLineEx(track->points[i], track->points[next], track->halfWidth * 3.0f, shoulderColor);
  }

  // Draw track surface as thick line segments
  for (int i = 0; i < track->pointCount; i++) {
    int next = (i + 1) % track->pointCount;
    DrawLineEx(track->points[i], track->points[next], track->halfWidth * 2.0f, trackColor);
  }

  // Draw track edges
  for (int i = 0; i < track->pointCount; i++) {
    int next = (i + 1) % track->pointCount;
    Vector2 a = track->points[i];
    Vector2 b = track->points[next];

    // Perpendicular direction
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) continue;
    float px = -dy / len;
    float py = dx / len;

    // Left edge
    Vector2 aL = { a.x + px * track->halfWidth, a.y + py * track->halfWidth };
    Vector2 bL = { b.x + px * track->halfWidth, b.y + py * track->halfWidth };
    DrawLineEx(aL, bL, 3.0f, edgeColor);

    // Right edge
    Vector2 aR = { a.x - px * track->halfWidth, a.y - py * track->halfWidth };
    Vector2 bR = { b.x - px * track->halfWidth, b.y - py * track->halfWidth };
    DrawLineEx(aR, bR, 3.0f, edgeColor);
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