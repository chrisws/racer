#include "audio.h"
#include <math.h>

static Sound engineSound;
static Sound beepSound;
static Sound skidSound;
static Sound aiSounds[3];
static bool enginePlaying = false;
static bool aiPlaying[3] = { false, false, false };

// Generate a tone wave with harmonics for a richer sound
static Wave GenerateToneWave(float freq, int sampleRate, int durationMs) {
  int numSamples = (sampleRate * durationMs) / 1000;
  Wave wave;
  wave.frameCount = numSamples;
  wave.sampleRate = sampleRate;
  wave.sampleSize = 16;
  wave.channels = 1;
  wave.data = (unsigned char *)MemAlloc(numSamples * 2);

  for (int i = 0; i < numSamples; i++) {
    float t = (float)i / sampleRate;
    float val = sinf(2.0f * PI * freq * t);
    val += 0.5f * sinf(2.0f * PI * freq * 2.0f * t);
    val += 0.25f * sinf(2.0f * PI * freq * 3.0f * t);
    val /= 1.75f;

    // Fade in/out to avoid clicks
    int fadeSamples = sampleRate / 10;
    float fade = 1.0f;
    if (i < fadeSamples) fade = (float)i / fadeSamples;
    if (i > numSamples - fadeSamples) fade = (float)(numSamples - i) / fadeSamples;
    val *= fade;

    ((short *)wave.data)[i] = (short)(val * 32767 * 0.3f);
  }
  return wave;
}

// Generate a noise wave for skid sounds
static Wave GenerateNoiseWave(int sampleRate, int durationMs) {
  int numSamples = (sampleRate * durationMs) / 1000;
  Wave wave;
  wave.frameCount = numSamples;
  wave.sampleRate = sampleRate;
  wave.sampleSize = 16;
  wave.channels = 1;
  wave.data = (unsigned char *)MemAlloc(numSamples * 2);

  for (int i = 0; i < numSamples; i++) {
    float val = ((float)GetRandomValue(0, 255) / 255.0f) - 0.5f;
    float fade = 1.0f - (float)i / numSamples;
    ((short *)wave.data)[i] = (short)(val * 32767 * 0.2f * fade);
  }
  return wave;
}

void AudioInit(void) {
  InitAudioDevice();

  // Engine: low rumble, 1-second loop
  Wave engineWave = GenerateToneWave(80.0f, 44100, 1000);
  engineSound = LoadSoundFromWave(engineWave);
  UnloadWave(engineWave);

  // AI car engines: slightly higher pitch, 1-second loop
  for (int i = 0; i < 3; i++) {
    Wave aiWave = GenerateToneWave(100.0f + i * 15.0f, 44100, 1000);
    aiSounds[i] = LoadSoundFromWave(aiWave);
    UnloadWave(aiWave);
  }

  // Start beep: short 880 Hz tone
  Wave beepWave = GenerateToneWave(880.0f, 44100, 200);
  beepSound = LoadSoundFromWave(beepWave);
  UnloadWave(beepWave);

  // Skid: 500 ms noise burst
  Wave skidWave = GenerateNoiseWave(44100, 500);
  skidSound = LoadSoundFromWave(skidWave);
  UnloadWave(skidWave);
}

void AudioShutdown(void) {
  StopSound(engineSound);
  UnloadSound(engineSound);
  for (int i = 0; i < 3; i++) {
    StopSound(aiSounds[i]);
    UnloadSound(aiSounds[i]);
  }
  UnloadSound(beepSound);
  UnloadSound(skidSound);
  CloseAudioDevice();
}

void AudioUpdateEngine(float speed, float maxSpeed) {
  if (speed > 5.0f) {
    if (!enginePlaying) {
      PlaySound(engineSound);
      enginePlaying = true;
    }
    // Restart the loop if it finished
    if (!IsSoundPlaying(engineSound)) {
      PlaySound(engineSound);
    }
    // Vary pitch with speed: 0.5x at idle → 2.0x at max
    float pitch = 0.5f + (speed / maxSpeed) * 1.5f;
    SetSoundPitch(engineSound, pitch);
  } else {
    if (enginePlaying) {
      StopSound(engineSound);
      enginePlaying = false;
    }
  }
}

void AudioPlayStartBeep(void) {
  PlaySound(beepSound);
}

void AudioPlaySkid(void) {
  if (!IsSoundPlaying(skidSound)) {
    PlaySound(skidSound);
  }
}

void AudioUpdateAICar(int index, float distance, float relativeSpeed, float aiSpeed) {
  if (index < 0 || index > 2) return;

  // Fade out beyond 500 units, max 50% volume
  float maxDist = 500.0f;
  float vol = 0.5f * (1.0f - distance / maxDist);
  if (vol < 0.0f) vol = 0.0f;

  if (vol < 0.02f || aiSpeed < 5.0f) {
    if (aiPlaying[index]) {
      StopSound(aiSounds[index]);
      aiPlaying[index] = false;
    }
    return;
  }

  if (!aiPlaying[index]) {
    PlaySound(aiSounds[index]);
    aiPlaying[index] = true;
  }
  // Restart loop if finished
  if (!IsSoundPlaying(aiSounds[index])) {
    PlaySound(aiSounds[index]);
  }

  // Doppler-like pitch: approaching = higher, receding = lower
  // relativeSpeed > 0 means closing in
  float pitch = 1.0f + (relativeSpeed / 400.0f) * 0.5f;
  if (pitch < 0.3f) pitch = 0.3f;
  if (pitch > 2.5f) pitch = 2.5f;
  SetSoundPitch(aiSounds[index], pitch);
  SetSoundVolume(aiSounds[index], vol);
}