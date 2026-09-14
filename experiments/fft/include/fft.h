#ifndef FFT_H
#define FFT_H

#include <stdint.h>
#include <stdio.h>

typedef struct wav_header* WaveHeader;
typedef struct fft_header* FFTheader;





WaveHeader openWaveFile(const char* filename);
void closeWaveFile(WaveHeader wh);
void waveFileInfo(WaveHeader wh);
uint32_t waveFileGetSampleRate(WaveHeader wh);
void waveFileSkipNsamples(WaveHeader wh, uint32_t samples);
uint32_t waveFileGetSamples(WaveHeader wh, int channel, float* buffer, uint32_t samples);




FFTheader initFFT(uint32_t samples, uint32_t n, WaveHeader wav);
void closeFFT(FFTheader fft);
void FFTgetFrameWindow(FFTheader fft, WaveHeader wav);
void FFTwriteFrequencies(FFTheader fft, FILE* fptr);

#endif