#include "track.h"
#include <math.h>

// Circuit with chicanes, kinks, and varied curves
static const Vector2 defaultTrackPoints[] = {
  { 300,  900 },   //  0: start/finish (bottom-left)
  { 650,  900 },   //  1: bottom straight
  { 900,  810 },   //  2: chicane dip
  { 1100, 900 },   //  3: chicane rise
  { 1400, 850 },   //  4: bottom-right approach
  { 1550, 650 },   //  5: right curve
  { 1480, 450 },   //  6: right kink (inward)
  { 1550, 250 },   //  7: right kink (outward)
  { 1350, 100 },   //  8: top-right
  { 1050, 180 },   //  9: top chicane entry
  { 850,  100 },   // 10: top chicane mid
  { 650,  180 },   // 11: top chicane exit
  { 350,  100 },   // 12: top-left
  { 150,  250 },   // 13: left curve
  { 200,  450 },   // 14: left kink (outward)
  { 150,  650 },   // 15: left kink (inward)
  { 250,  850 },   // 16: bottom-left curve
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