#include <stdio.h>
#include <stdint.h>
#include "synth_internal.h"

#define BUFFER_FRAMES 1024


static void writeWavHeader(FILE* fptr, uint32_t sampleRate) {
    if (!fptr) return;

    uint16_t bitsPerSample = 16;
    uint16_t channels = 2;

    uint8_t header[44] = {'R', 'I', 'F', 'F',   0, 0, 0, 0,   'W', 'A', 'V', 'E',   'f', 'm', 't', ' ',   16, 0, 0, 0,   1, 0,   0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0,   0, 0,   'd', 'a', 't', 'a',   0, 0, 0, 0};
    uint32_t dataSize = 0; //(bitsPerSample>>3)*channels*samples; !!!
    uint32_t sizeOfFile = dataSize+36; //                         !!!
    uint32_t bytesPerSec = sampleRate*channels*(bitsPerSample>>3);
    uint8_t blockAlign = channels*(bitsPerSample>>3);

    int i;
    for (i=0; i<4; i++) header[4+i] = (uint8_t)(((sizeOfFile >> (8*i))&0xFF));
    for (i=0; i<2; i++) header[22+i] = (uint8_t)(((channels >> (8*i))&0xFF));
    for (i=0; i<4; i++) header[24+i] = (uint8_t)(((sampleRate >> (8*i))&0xFF));
    for (i=0; i<4; i++) header[28+i] = (uint8_t)(((bytesPerSec >> (8*i))&0xFF));
    for (i=0; i<2; i++) header[32+i] = (uint8_t)(((blockAlign >> (8*i))&0xFF));
    for (i=0; i<2; i++) header[34+i] = (uint8_t)(((bitsPerSample >> (8*i))&0xFF));
    for (i=0; i<4; i++) header[40+i] = (uint8_t)(((dataSize >> (8*i))&0xFF));

    for (i=0; i<44; i++) fputc((int)(header[i]), fptr);
}


static void updateWavHeader(FILE* fptr, uint32_t samples) {
    uint16_t bitsPerSample = 16;
    uint16_t channels = 2;

    uint32_t dataSize = (bitsPerSample>>3)*channels*samples;
    uint32_t sizeOfFile = dataSize+36;

    fseek(fptr, 4, SEEK_SET);
    for (int i=0; i<4; i++) fputc((int)((sizeOfFile >> (8*i))&0xFF), fptr);

    fseek(fptr, 40, SEEK_SET);
    for (int i=0; i<4; i++) fputc((int)((dataSize >> (8*i))&0xFF), fptr);
}


static uint32_t simulateAudio(FILE* fptr) {
    if (fptr) return 0;
    return 0;
}


int exportProjectAsWave(const char* filepath) {
    if (!filepath) return 1;

    FILE* fptr = fopen(filepath, "rb");
    if (!fptr) return 1;

    writeWavHeader(fptr, 44100);
    uint32_t samples = simulateAudio(fptr);
    updateWavHeader(fptr, samples);

    fclose(fptr);
    return 0;
}