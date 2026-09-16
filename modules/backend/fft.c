#include <backend.h>
#include <synth.h>
#include <complex.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <visualizer.h>


#define M_TAU 6.28318530717958647
#define SAMPLES_WINDOW_SIZE 2048
#define SAMPLE_RATE 44100


static volatile float wavBuffer[SAMPLES_WINDOW_SIZE]={0};
static FFTheader fft = NULL;



void recordAudioFrames(float* buffer, int samples) {
    int startingFrameIdx=0;

    if (samples>SAMPLES_WINDOW_SIZE) {
        startingFrameIdx=samples-SAMPLES_WINDOW_SIZE;
        samples = SAMPLES_WINDOW_SIZE;

        for (int i=0; i<samples; i++) {
            int idx = ((startingFrameIdx+i)<<1);
            ((float*)wavBuffer)[i] = 0.5*(buffer[idx]+buffer[idx+1]);
        }
    } else {
        int d = SAMPLES_WINDOW_SIZE-samples;
        memmove(((float*)wavBuffer), ((float*)wavBuffer)+samples, d*sizeof(float));
        for (int i=0; i<samples; i++) {
            ((float*)wavBuffer)[i+d] = 0.5*(buffer[i<<1]+buffer[1+(i<<1)]);
        }
    }
}


int FFTinit() {
    if (fft) FFTcloseHeader(fft);
    fft = FFTcreateHeader();

    return 0;
}

int FFTclose() {
    if (fft) FFTcloseHeader(fft);
    fft = NULL;

    return 0;
}


typedef struct fft_data {
    int N;
    int freqN;
    Complex* intensity;
    float* smooth;
}* FFTheader;

FFTheader FFTcreateHeader() {
    FFTheader ret = malloc(sizeof(struct fft_data));
    if (!ret) return NULL;

    ret->N = SAMPLES_WINDOW_SIZE;
    ret->freqN = SAMPLES_WINDOW_SIZE/2;
    ret->intensity = calloc(ret->N, sizeof(Complex));
    if (!(ret->intensity)) {
        free(ret);
        return NULL;
    }
    ret->smooth = calloc(ret->freqN, sizeof(float));
    if (!(ret->smooth)) {
        free(ret->intensity);
        free(ret);
        return NULL;
    }
    return ret;
}

void FFTcloseHeader(FFTheader fft) {
    if (!fft) return;
    if (fft->intensity) free(fft->intensity);
    fft->intensity = NULL;
    if (fft->smooth) free(fft->smooth);
    fft->smooth = NULL;
    free(fft);
}


static Complex _cooleyTukeyFFT(uint32_t k, volatile float* buffer, uint32_t stepSize, uint32_t num) {
    if (num==0) return (Complex){0,0};
    double phase = M_TAU*k/(double)num;

    if (num==1) return (Complex){buffer[0], 0};

    uint32_t half = num>>1;
    Complex even = _cooleyTukeyFFT(k%half, buffer, stepSize<<1, half);
    Complex odd = _cooleyTukeyFFT(k%half, buffer+stepSize, stepSize<<1, half);
    
    odd = complexProduct(odd, (Complex){cos(phase), -sin(phase)});
    return complexAddition(even, odd);
}


void FFTupdateFrameWindow(FFTheader fft) {
    if (!fft) return;

    uint32_t N = SAMPLES_WINDOW_SIZE;
    uint32_t freqN = fft->freqN;
    for (uint32_t k=0; k<freqN; k++) {
        Complex z = _cooleyTukeyFFT(k, wavBuffer, 1, N);
        fft->intensity[k] = z;
        fft->smooth[k] += 0.2*(complexMagnitude(z)-fft->smooth[k]);
    }
}


static uint32_t _rev(uint32_t bits, uint32_t logn) {
    uint32_t ret = 0;
    for (uint32_t i=0; i<logn; i++) {
        ret |= (((bits>>i)&0x1)<<(logn-1-i));
    }
    return ret;
}

static void _bitReverseCopy(uint32_t n, float* buffa, Complex* buffA) {
    if (!buffa || !buffA) return;
    if (n&(n-1)) return;

    uint32_t lg2n = (uint32_t)log2(n);

    for (uint32_t k=0; k<n; k++) {
        uint32_t nidx = _rev(k, lg2n);
        buffA[nidx] = (Complex){buffa[k], 0.0};
    }
}

// https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm#Data_reordering,_bit_reversal,_and_in-place_algorithms
void _wikipediaAlgorithm(FFTheader fft, float* maxIntensity) {
    if (!fft) return;

    volatile float* a = wavBuffer;
    Complex* A = fft->intensity;
    uint32_t N = fft->N;
    uint32_t N2 = fft->freqN;
    uint32_t log2N = (uint32_t)log2(N);

    _bitReverseCopy(N, (float*)a, A);

    for (uint32_t s=1; s<=log2N; s++) {
        uint32_t m = (1<<s);
        double phase = M_TAU/m;
        Complex omega_m = {cos(phase), -sin(phase)};
        for (uint32_t k=0; k<N; k+=m) {
            Complex omega = {1.0, 0.0};
            uint32_t target = (m>>1);
            for (uint32_t j=0; j<target; j++) {
                Complex t = complexProduct(omega, A[k+j+target]);
                Complex u = A[k+j];
                
                A[k+j] = complexAddition(u, t);
                A[k+j+target] = complexSubtraction(u, t);

                omega = complexProduct(omega, omega_m);
            }
        }
    }

    float maxInt = 0;
    for (uint32_t k=0; k<N2; k++) {
        fft->smooth[k] += 0.2*(complexMagnitude(fft->intensity[k])-fft->smooth[k]);
        if (fft->smooth[k]>maxInt) maxInt=fft->smooth[k];
    }
    if (maxIntensity) *maxIntensity = maxInt;
}


void FFTupdate(float* maxIntensity) {
    _wikipediaAlgorithm(fft, maxIntensity);
}

float* FFTgetIntensityBufferForHeader(FFTheader fft, int* num) {
    if (!fft) return NULL;
    *num = fft->freqN;
    return fft->smooth;
}

float* FFTgetIntensityBuffer(int* num) {
    return FFTgetIntensityBufferForHeader(fft, num);
}

int FFTgetBufferLength() {
    if (fft) return fft->freqN;
    else return SAMPLES_WINDOW_SIZE/2;
}

float FFTgetDeltaFrequency() {
    return SAMPLE_RATE/(float)SAMPLES_WINDOW_SIZE;
}