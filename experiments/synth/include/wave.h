#ifndef WAVE_H
#define WAVE_H

#include <stdint.h>
#include <stdio.h>

typedef struct wav_header* WaveHeader;


enum wave_sample_type {
    MONO_UI8,
    MONO_UI16,
    MONO_FLOAT,

    STEREO_UI8,
    STEREO_UI16,
    STEREO_FLOAT,
};


WaveHeader openWaveFile(const char* filename);
void closeWaveFile(WaveHeader wh);
void waveFileInfo(WaveHeader wh);
uint32_t waveFileGetSampleRate(WaveHeader wh);
void waveFileSkipNsamples(WaveHeader wh, uint32_t samples);
uint32_t waveFileGetSamples(WaveHeader wh, int channel, float* buffer, uint32_t samples);
WaveHeader createWaveFile(const char* filename, uint32_t sampleRate, int channels, int bitsPerSample);
void finishWaveFile(WaveHeader wh);
void waveFileAddSamples(WaveHeader wh, void* buffer, int samples, enum wave_sample_type sampleType);

#endif