#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "synth_internal.h"
#include "backend_internal.h"
#include <math.h>


#define BUFFER_FRAMES 4096


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



static int noteCompare(const void* a, const void* b) {
    if (!a && !b) return 0;
    if (!a || !b) return a?-1:1;
    
    Note na = *(Note*)a;
    Note nb = *(Note*)b;

    if (na->timestamp>nb->timestamp) return 1;
    if (na->timestamp<nb->timestamp) return -1;
    return 0;
}

static void sortNotes(Note* buffer, uint32_t elements) {
    if (!buffer || !elements) return;
    qsort(buffer, elements, sizeof(Note), noteCompare);
}

static uint32_t simulateAudio(FILE* fptr, uint32_t sampleRate) {
    uint32_t notesNum = 0;
    uint16_t tracksNum = globalProject->tracksNum;
    for (uint16_t i=0; i<tracksNum; i++) notesNum+=(globalProject->tracks)[i].numElements;

    uint32_t eventsNum = (notesNum<<1);
    Note noteOffEvents = malloc(notesNum*sizeof(struct note_data));
    Note* allEvents = malloc(eventsNum*sizeof(Note));

    if (!noteOffEvents || !allEvents) {
        if (noteOffEvents) free(noteOffEvents);
        if (allEvents) free(allEvents);
        return 0;
    }

    uint32_t noteIdx=0;
    for (uint16_t i=0; i<tracksNum; i++) {
        uint32_t curNotesNum = (globalProject->tracks)[i].numElements;
        Note* curNotes = (globalProject->tracks)[i].notes;
        for (uint32_t j=0; j<curNotesNum; j++) {
            Note nt = curNotes[j];
            nt->track = (uint8_t)i;

            noteOffEvents[noteIdx] = *nt;
            noteOffEvents[noteIdx].timestamp += noteOffEvents[noteIdx].duration;

            allEvents[noteIdx] = nt;                                    // Note on
            allEvents[noteIdx+notesNum] = noteOffEvents+noteIdx;        // Note off

            noteIdx++;
        }
    }

    sortNotes(allEvents, eventsNum);    

    double pieceToSample = timestampPiecesToSamples(1, sampleRate);
    double sampleToPieces = samplesToTimestampPieces(1, sampleRate);
    uint32_t eventIdx=0, sampleIdx=0, curTimestamp=0, frames=0;
    double fCurTimestamp=0, fframes=0;

    int16_t* wavBuffer = malloc(BUFFER_FRAMES*2*sizeof(int16_t));
    if (!wavBuffer) {
        free(noteOffEvents);
        free(allEvents);
        return 0;
    }

    exportSynthPanic();

    uint32_t samples=15*BUFFER_FRAMES;
    for (int i=0; i<5; i++) {
        renderExportAudio(wavBuffer, BUFFER_FRAMES);
        fwrite(wavBuffer, sizeof(int16_t)*2, BUFFER_FRAMES, fptr);
    }

    while (eventIdx<eventsNum) {
        while (eventIdx<eventsNum && allEvents[eventIdx]->timestamp<=curTimestamp) {
            Note nt = allEvents[eventIdx++];
            Track tr = globalProject->tracks+nt->track;
            if (nt>=noteOffEvents+notesNum || nt<noteOffEvents) {
                exportSynthProgramNoteOnPanning(nt->key, nt->velocity*0.007874*tr->velocity, tr->program, tr->panning);
            } else {
                exportSynthProgramNoteOffPanning(nt->key, tr->program);
            }
        }

        frames = 0;
        if (eventIdx>=eventsNum) break;

        fframes = ceil(pieceToSample*(allEvents[eventIdx]->timestamp-curTimestamp));
        if (fframes>BUFFER_FRAMES) frames = BUFFER_FRAMES;
        else frames = (uint32_t)fframes;

        samples += frames;
        sampleIdx += frames;

        fCurTimestamp = sampleIdx*sampleToPieces;
        curTimestamp = (uint32_t)fCurTimestamp;

        renderExportAudio(wavBuffer, frames);
        fwrite(wavBuffer, sizeof(int16_t)*2, frames, fptr);
    }

    exportSynthPanic();

    for (int i=0; i<10; i++) {
        renderExportAudio(wavBuffer, BUFFER_FRAMES);
        fwrite(wavBuffer, sizeof(int16_t)*2, BUFFER_FRAMES, fptr);
    }

    free(wavBuffer);
    free(noteOffEvents);
    free(allEvents);

    return samples;
}


int exportProjectAsWave(const char* filename) {         // In the future, I might make it in a different thread
    if (!filename) return 1;

    FILE* fptr = fopen(filename, "wb");
    if (!fptr) return 1;

    uint32_t sampleRate = 44100;
    writeWavHeader(fptr, sampleRate);
    uint32_t samples = simulateAudio(fptr, sampleRate);
    updateWavHeader(fptr, samples);

    fclose(fptr);
    return (samples==0);
}