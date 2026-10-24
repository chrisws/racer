#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"

#ifdef __cplusplus
extern "C" {
#endif

void AudioInit(void);
void AudioShutdown(void);
void AudioUpdateEngine(float speed, float maxSpeed);
void AudioUpdateAICar(int index, float distance, float relativeSpeed, float aiSpeed);
void AudioPlayStartBeep(void);
void AudioPlayCountdownBeep(int remaining);
void AudioPlaySkid(void);
void AudioPlayChime(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_H