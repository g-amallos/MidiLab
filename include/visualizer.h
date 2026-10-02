#ifndef VISUALIZER_H
#define VISUALIZER_H
#include <stdint.h>



/* FFT  (modules/backend/fft.c) */

typedef struct fft_data* FFTheader;


int FFTinit();
int FFTclose();

FFTheader FFTcreateHeader();
void FFTcloseHeader(FFTheader fft);
void FFTupdateFrameWindow(FFTheader fft);
void FFTupdate(float* maxIntensity);
float* FFTgetIntensityBufferForHeader(FFTheader fft, int* num);
float* FFTgetIntensityBuffer(int* num);
int FFTgetBufferLength();
float FFTgetDeltaFrequency();
void FFTzeroOutBuffers();


int FFTexportInit();
int FFTexportClose();
void recordExportAudioFramesInt16(int16_t* buffer, int samples);
void recordExportAudioFramesFloat(float* buffer, int samples);
void recordExportAudioSilence(int samples);
float* FFTexportGetIntensityBuffer(int* num);
void FFTexportZeroOutBuffers();
void FFTexportUpdate(float* maxIntensity);



/* Visualizer  (interface/render/visualizer.c) */

void visualizationRenderSimInit();
void visualizationRenderSimClose();
void visualizationRenderSimPrecomputeValues(float maxIntensity);
void visualizationRenderSimRender();
Image visualizationSimGetImage();


#endif