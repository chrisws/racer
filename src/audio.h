#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"

void AudioInit(void);
void AudioShutdown(void);
void AudioUpdateEngine(float speed, float maxSpeed);
void AudioUpdateAICar(int index, float distance, float relativeSpeed, float aiSpeed);
void AudioPlayStartBeep(void);
void AudioPlaySkid(void);

#endif // AUDIO_H