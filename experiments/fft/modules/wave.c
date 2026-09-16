#include <stdlib.h>
#include <fileops.h>
#include <stdint.h>
#include <fft.h>
#include <string.h>


struct wav_header {
    uint8_t channels;
    uint8_t bitsPerChannelSample;
    uint8_t bytesPerChannelSample;
    uint8_t bytesPerSample;


    uint32_t sampleRate;
    uint32_t totalSamples;
    uint32_t fileSize;

    FILE* fptr;
};


static int _readWaveHeader(FILE* fptr, struct wav_header* header) {
    if (!fptr || !header) return 1;

    const char* riff = "RIFF";
    const char* wave = "WAVE";
    const char* fmt = "fmt ";
    const char* data = "data";



    char buff[5]={0,};
    int ret=0;
    
    ret += readString(fptr, buff, 4);
    if (strcmp(riff, buff)) return 1;

    uint32_t fileSize = 0;
    ret += readUint32(fptr, &fileSize);

    ret += readString(fptr, buff, 4);
    if (strcmp(wave, buff)) return 1;

    ret += readString(fptr, buff, 4);
    if (strcmp(fmt, buff)) return 1;

    uint32_t formatData=0;
    ret += readUint32(fptr, &formatData);

    uint16_t typeOfFormat=0;
    ret += readUint16(fptr, &typeOfFormat);

    uint16_t channels=0;
    ret += readUint16(fptr, &channels);

    uint32_t sampleRate=0;
    ret += readUint32(fptr, &sampleRate);

    uint32_t bitsPerSecond=0;
    ret += readUint32(fptr, &bitsPerSecond);

    uint16_t bytesPerSample=0;
    ret += readUint16(fptr, &bytesPerSample);

    uint16_t bitsPerSample=0;
    ret += readUint16(fptr, &bitsPerSample);

    ret += readString(fptr, buff, 4);
    if (strcmp(data, buff)) return 1;

    uint32_t dataSize=0;
    ret += readUint32(fptr, &dataSize);

    if (ret) return ret;

    header->channels = channels;
    header->bitsPerChannelSample = bitsPerSample;
    header->bytesPerChannelSample = (bitsPerSample>>3);
    header->bytesPerSample = bytesPerSample;
    header->sampleRate = sampleRate;
    header->totalSamples = dataSize/bytesPerSample;
    header->fileSize = fileSize;
    return 0;
}



WaveHeader openWaveFile(const char* filename) {
    if (!filename) return NULL;
    FILE* fptr = fopen(filename, "rb");
    if (!fptr) return NULL;

    struct wav_header ret = {0,};
    int r = _readWaveHeader(fptr, &ret);
    if (r) {
        fclose(fptr);
        return NULL;
    }

    WaveHeader toRet = malloc(sizeof(struct wav_header));
    if (!toRet) {
        fclose(fptr);
        return NULL;
    }

    *toRet = ret;
    toRet->fptr = fptr;
    return toRet;
}

void closeWaveFile(WaveHeader wh) {
    if (!wh) return;

    if (wh->fptr) fclose(wh->fptr);
    wh->fptr = NULL;
    free(wh);
}

uint32_t waveFileGetSampleRate(WaveHeader wh) {
    if (!wh) return 0;
    return wh->sampleRate;
}

void waveFileInfo(WaveHeader wh) {
    if (!wh) return;
    printf("Channels: %u\n", wh->channels);
    printf("Bits Per Channel Sample: %u\n", wh->bitsPerChannelSample);
    printf("Bytes Per Channel Sample: %u\n", wh->bytesPerChannelSample);
    printf("Bytes Per Sample: %u\n", wh->bytesPerSample);
    printf("Sample Rate: %u\n", wh->sampleRate);
    printf("Total Samples: %u\n", wh->totalSamples);
    printf("Total Duration: %lf seconds\n", wh->totalSamples/(double)(wh->sampleRate));
    printf("File Size: %u\n", wh->fileSize);
}

void waveFileSkipNsamples(WaveHeader wh, uint32_t samples) {
    if (!wh) return;

    uint8_t trash=0, bytesPerChannelSample=wh->bytesPerChannelSample;
    uint16_t channels=wh->channels;
    FILE*  fptr = wh->fptr;

    for (uint32_t i=0; i<samples; i++) {
        for (int j=0; j<channels; j++) {
            for (int b=0; b<bytesPerChannelSample; b++) if (readUint8(fptr, &trash)) return;
        }
    }
}

uint32_t waveFileGetSamples(WaveHeader wh, int channel, float* buffer, uint32_t samples) {
    if (!wh || channel<0 || channel>=wh->channels) return 0;

    uint8_t trash=0, channels=wh->channels;
    uint8_t bytesPerChannelSample = wh->bytesPerChannelSample;
    FILE*  fptr = wh->fptr;

    uint32_t renSamps = 0;
    for (uint32_t i=0; i<samples; i++) {
        for (int j=0; j<channel; j++) {
            for (int b=0; b<bytesPerChannelSample; b++) if (readUint8(fptr, &trash)) return renSamps;
        }
        if (bytesPerChannelSample==1) {
            uint8_t spl = 0;
            if (readUint8(fptr, &spl)) return renSamps;
            float s = (spl-128)/128.0;
            buffer[renSamps++] = s;
        } else if (bytesPerChannelSample==2) {
            int16_t spl = 0;
            if (readUint16(fptr, (uint16_t*)&spl)) return renSamps;
            float s = spl/32768.0;
            buffer[renSamps++] = s;
        }
        for (int j=channel+1; j<channels; j++) {
            for (int b=0; b<bytesPerChannelSample; b++) if (readUint8(fptr, &trash)) return renSamps;
        }
    }
    return renSamps;
}