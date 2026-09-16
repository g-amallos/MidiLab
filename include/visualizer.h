#ifndef VISUALIZER_H
#define VISUALIZER_H


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

#endif