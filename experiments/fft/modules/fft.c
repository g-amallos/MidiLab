#include <stdlib.h>
#include <math.h>
#include <fft.h>
#include <complex.h>


#define M_TAU 6.28318530717958647

struct fft_freq_bin {
    float freq;
    float real;
    float imaginary;
    float value;
};


struct fft_header {
    uint32_t sampleRate;

    uint32_t sampleCapacity;
    uint32_t samplesWritten;
    float* buffer;

    uint32_t freqN;
    struct fft_freq_bin* frequencies;
};



FFTheader initFFT(uint32_t samples, uint32_t numOfFreqs, WaveHeader wav) {
    if (!wav) return NULL;
    if (numOfFreqs!=samples/2) return NULL;
    if (numOfFreqs>10000 || samples>100000) return NULL;

    struct fft_freq_bin* freqs = malloc(numOfFreqs*sizeof(struct fft_freq_bin));
    if (!freqs) return NULL;

    uint32_t sampleRate = waveFileGetSampleRate(wav);

    for (uint32_t i=0; i<numOfFreqs; i++) {
        freqs[i].freq = i*sampleRate/(float)samples;
        freqs[i].real = 0;
        freqs[i].imaginary = 0;
        freqs[i].value = 0;
    }

    float* buffer = calloc(samples, sizeof(float));
    if (!buffer) {
        free(freqs);
        return NULL;
    }

    FFTheader ret = malloc(sizeof(struct fft_header));
    if (!ret) {
        free(freqs);
        free(buffer);
        return NULL;
    }

    ret->sampleRate = sampleRate;
    ret->sampleCapacity = samples;
    ret->samplesWritten = 0;
    ret->buffer = buffer;
    ret->freqN = numOfFreqs;
    ret->frequencies = freqs;

    return ret;
}

void closeFFT(FFTheader fft) {
    if (!fft) return;
    if (fft->buffer) free(fft->buffer);
    if (fft->frequencies) free(fft->frequencies);

    fft->buffer = NULL;
    fft->frequencies = NULL;

    fft->sampleRate = 0;
    fft->sampleCapacity = 0;
    fft->samplesWritten = 0;
    fft->freqN = 0;

    free(fft);
}



static Complex _cooleyTukeyFFT(uint32_t k, float* buffer, uint32_t stepSize, uint32_t num) {
    if (num==0) return (Complex){0,0};
    double phase = M_TAU*k/(double)num;

    if (num==1) return (Complex){buffer[0], 0};

    uint32_t half = num>>1;
    Complex even = _cooleyTukeyFFT(k%half, buffer, stepSize<<1, half);
    Complex odd = _cooleyTukeyFFT(k%half, buffer+stepSize, stepSize<<1, half);
    
    odd = complexProduct(odd, (Complex){cos(phase), -sin(phase)});
    return complexAddition(even, odd);
}


void FFTgetFrameWindow(FFTheader fft, WaveHeader wav) {
    if (!fft || !wav) return;

    fft->samplesWritten = waveFileGetSamples(wav, 0, fft->buffer, fft->sampleCapacity);
    if (fft->samplesWritten != fft->sampleCapacity) return;

    float* samples = fft->buffer;
    uint32_t N = fft->sampleCapacity;
    uint32_t freqN = fft->freqN;
    for (uint32_t k=0; k<freqN; k++) {
        Complex z = _cooleyTukeyFFT(k, samples, 1, N);
        fft->frequencies[k].real = z.real;
        fft->frequencies[k].imaginary = z.imaginary;
        fft->frequencies[k].value = 20.0*log10(complexMagnitude(z)+1e-8);
    }
}


static uint32_t _rev(uint32_t bits, uint32_t logn) {
    uint32_t ret = 0;
    for (int i=0; i<logn; i++) {
        ret |= (((bits>>i)&0x1)<<(logn-1-i));
    }
    return ret;
}

static void _bitReverseCopy(uint32_t n, Complex* buffa, Complex* buffA) {
    if (!buffa || !buffA) return;
    if (n&(n-1)) return;

    uint32_t lg2n = (uint32_t)log2(n);

    for (uint32_t k=0; k<n; k++) {
        uint32_t nidx = _rev(k, lg2n);
        buffA[nidx] = buffa[k];
    }
}


void _wikipediaAlgorithm(FFTheader fft, WaveHeader wav) {
    if (!fft || !wav) return;

    fft->samplesWritten = waveFileGetSamples(wav, 0, fft->buffer, fft->sampleCapacity);
    if (fft->samplesWritten != fft->sampleCapacity) return;

    float* a = fft->buffer;
    struct fft_freq_bin* A = fft->frequencies;
    uint32_t N = fft->sampleCapacity;
    uint32_t freqN = fft->freqN;
    uint32_t log2N = (uint32_t)log2(N);

    //bit-reverse-copy(a, A)

    for (uint32_t s=1; s<=log2N; s++) {
        uint32_t m = (1<<s);
        double phase = M_TAU/m;
        Complex omega_m = {cos(phase), -sin(phase)};
        for (uint32_t k=0; k<N; k+=m) {
            Complex omega = {1.0, 0.0};
            uint32_t target = (m>>1);
            for (uint32_t j=0; j<target; j++) {
                struct fft_freq_bin fftb1=A[k+j+target], fftb2=A[k+j];
                Complex t = complexProduct(omega, (Complex){fftb1.real, fftb1.imaginary});
                Complex u = (Complex){fftb2.real, fftb2.imaginary};
                
                Complex tmp1 = complexAddition(u, t);
                A[k+j].real = tmp1.real;
                A[k+j].imaginary = tmp1.imaginary;

                Complex tmp2 = complexSubtraction(u, t);
                A[k+j+target].real = tmp2.real;
                A[k+j+target].imaginary = tmp2.imaginary;

                omega = complexProduct(omega, omega_m);
            }
        }
    }


}

/*
void FFTgetFrameWindow(FFTheader fft, WaveHeader wav) {
    if (!fft || !wav) return;

    fft->samplesWritten = waveFileGetSamples(wav, 0, fft->buffer, fft->sampleCapacity);
    if (fft->samplesWritten != fft->sampleCapacity) return;

    float* samples = fft->buffer;
    uint32_t N = fft->sampleCapacity;
    uint32_t freqN = fft->freqN;
    for (uint32_t k=0; k<freqN; k++) {
        double real=0, imag=0;
        for (uint32_t n=0; n<N; n++) {
            double phase = M_TAU/N*n*k;
            real += samples[n]*cos(phase);
            imag -= samples[n]*sin(phase);
        }
        
        fft->frequencies[k].real = (float)real;
        fft->frequencies[k].imaginary = (float)imag;
        fft->frequencies[k].value = (float)sqrt(real*real+imag*imag);

        fft->frequencies[k].value = 20.0*log10(fft->frequencies[k].value+1e-8);
    }
}*/


void FFTwriteFrequencies(FFTheader fft, FILE* fptr) {
    if (!fptr || !fft) return;

    fprintf(fptr, "#---- Info ----#\nSample Rate: %u\nSamples: %u\nWindow Duration: %.5lf seconds\n\n", fft->sampleRate, fft->sampleCapacity, fft->sampleCapacity/(double)fft->sampleRate);
    uint32_t freqN = fft->freqN;
    for (uint32_t i=0; i<freqN; i++) {
        fprintf(fptr, "Freq (#%5u): %8.3lf Hz   %10.6lf\n", i, fft->frequencies[i].freq, fft->frequencies[i].value);
    }
    return;
}