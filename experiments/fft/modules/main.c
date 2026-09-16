#include <fft.h>
#include <stdio.h>



int main(int argc, char** argv) {
    if (argc<3) {
        fprintf(stderr, "Usage: `fft assets/A4.wav assets/test.txt`\n");
        return 1;
    }

    WaveHeader wh = openWaveFile(argv[1]);
    if (!wh) {
        fprintf(stderr, "Couldn't open `%s` correctly\n", argv[1]);
        return 2;
    }

    waveFileInfo(wh);
    waveFileSkipNsamples(wh, (uint32_t)(0.5*waveFileGetSampleRate(wh)));

    FFTheader fft = initFFT(4096, 2048, wh);
    FFTgetFrameWindow(fft, wh);
    FILE* fptr = fopen(argv[2], "w");
    if (!fptr) {
        fprintf(stderr, "Couldn't open `%s` correctly\n", argv[2]);
        closeFFT(fft);
        closeWaveFile(wh);
        return 2;
    }

    FFTwriteFrequencies(fft, fptr);
    closeFFT(fft);
    closeWaveFile(wh);

    return 0;
}