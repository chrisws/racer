#include "audio.h"
#include <math.h>

static Sound engineSound;
static Sound beepSound;
static Sound skidSound;
static Sound chimeSound;
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

  // Chime: two-tone "cha-ching" (1500 Hz 80ms + 2200 Hz 220ms)
  {
    int sr = 44100;
    int dur1 = 80, dur2 = 220;
    int n1 = (sr * dur1) / 1000;
    int n2 = (sr * dur2) / 1000;
    int total = n1 + n2;
    Wave chimeWave;
    chimeWave.frameCount = total;
    chimeWave.sampleRate = sr;
    chimeWave.sampleSize = 16;
    chimeWave.channels = 1;
    chimeWave.data = (unsigned char *)MemAlloc(total * 2);
    for (int i = 0; i < total; i++) {
      float t = (float)i / sr;
      float val;
      if (i < n1) {
        val = sinf(2.0f * PI * 1500.0f * t);
        val += 0.4f * sinf(2.0f * PI * 3000.0f * t);
        val /= 1.4f;
        // quick fade-out at end of "cha"
        int rem = n1 - i;
        if (rem < sr / 20) val *= (float)rem / (sr / 20);
      } else {
        float t2 = (float)(i - n1) / sr;
        val = sinf(2.0f * PI * 2200.0f * t2);
        val += 0.5f * sinf(2.0f * PI * 4400.0f * t2);
        val += 0.2f * sinf(2.0f * PI * 6600.0f * t2);
        val /= 1.7f;
        // fade-out at end of "ching"
        int rem = total - i;
        int fade = sr / 8;
        if (rem < fade) val *= (float)rem / fade;
      }
      ((short *)chimeWave.data)[i] = (short)(val * 32767 * 0.4f);
    }
    chimeSound = LoadSoundFromWave(chimeWave);
    UnloadWave(chimeWave);
  }
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
  UnloadSound(chimeSound);
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

void AudioPlayCountdownBeep(int remaining) {
  if (remaining > 3) {
    // "Get ready" — lower, softer
    SetSoundPitch(beepSound, 0.75f);
    SetSoundVolume(beepSound, 0.5f);
  } else if (remaining > 0) {
    // F1 lights — sharp, loud
    SetSoundPitch(beepSound, 1.0f);
    SetSoundVolume(beepSound, 0.9f);
  } else {
    // "GO!" — bright, high
    SetSoundPitch(beepSound, 1.5f);
    SetSoundVolume(beepSound, 1.0f);
  }
  PlaySound(beepSound);
}

void AudioPlaySkid(void) {
  if (!IsSoundPlaying(skidSound)) {
    PlaySound(skidSound);
  }
}

void AudioPlayChime(void) {
  SetSoundPitch(chimeSound, 1.0f);
  SetSoundVolume(chimeSound, 0.7f);
  PlaySound(chimeSound);
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