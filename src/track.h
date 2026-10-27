#ifndef TRACK_H
#define TRACK_H

#include "raylib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_TRACK_POINTS 512

typedef struct {
  Vector2 points[MAX_TRACK_POINTS];
  int pointCount;
  float halfWidth;   // half the track width
} Track;

void TrackInit(Track *track);
void TrackSmooth(Track *track, int subdivisions);
void TrackDraw(const Track *track);
int TrackIsOnTrack(const Track *track, Vector2 pos);
float TrackGetNearestDistance(const Track *track, Vector2 pos);
Vector2 TrackGetNearestPoint(const Track *track, Vector2 pos);
int TrackGetNearestSegment(const Track *track, Vector2 pos);

#ifdef __cplusplus
}
#endif

#endif // TRACK_H