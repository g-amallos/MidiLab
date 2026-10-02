#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../synth_internal.h"
#include "../backend_internal.h"
#include <math.h>
#include <threads.h>
#include <utils.h>
#include <lame.h>



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

    if (na->velocity>nb->velocity) return 1;
    if (na->velocity<nb->velocity) return -1;

    if (na->key>nb->key) return 1;
    if (na->key<nb->key) return -1;

    return 0;
}

static void sortNotes(Note* buffer, uint32_t elements) {
    if (!buffer || !elements) return;
    qsort(buffer, elements, sizeof(Note), noteCompare);
}

static Note* _generateEventArray(uint32_t* notesNum, uint32_t* eventsNum, Note* noteOffEvents) {
    uint32_t locNotesNum = 0;
    uint16_t tracksNum = globalProject->tracksNum;
    for (uint16_t i=0; i<tracksNum; i++) locNotesNum+=(globalProject->tracks)[i].numElements;

    *notesNum = locNotesNum;

    uint32_t locEventsNum = (locNotesNum<<1);
    *eventsNum = locEventsNum;
    Note locNoteOffEvents = malloc(locNotesNum*sizeof(struct note_data));
    Note* allEvents = malloc(locEventsNum*sizeof(Note));

    if (!locNoteOffEvents || !allEvents) {
        if (locNoteOffEvents) free(locNoteOffEvents);
        if (allEvents) free(allEvents);
        return NULL;
    }

    uint32_t noteIdx=0;
    for (uint16_t i=0; i<tracksNum; i++) {
        uint32_t curNotesNum = (globalProject->tracks)[i].numElements;
        Note* curNotes = (globalProject->tracks)[i].notes;
        for (uint32_t j=0; j<curNotesNum; j++) {
            Note nt = curNotes[j];
            nt->track = (uint8_t)i;

            locNoteOffEvents[noteIdx] = *nt;
            locNoteOffEvents[noteIdx].timestamp += locNoteOffEvents[noteIdx].duration;
            locNoteOffEvents[noteIdx].velocity = 0;

            allEvents[noteIdx] = nt;                                    // Note on
            allEvents[noteIdx+locNotesNum] = locNoteOffEvents+noteIdx;        // Note off

            noteIdx++;
        }
    }

    *noteOffEvents = locNoteOffEvents;

    sortNotes(allEvents, locEventsNum);
    return allEvents;
}

static uint32_t simulateAudio(FILE* fptr, uint32_t sampleRate, float jobPercentage, float jobOffset) {
    uint32_t notesNum=0, eventsNum=0;
    Note noteOffEvents=NULL;
    Note* allEvents = _generateEventArray(&notesNum, &eventsNum, &noteOffEvents);
    if (!allEvents) return 0;
    
    double pieceToSample = timestampPiecesToSamples(1, sampleRate);
    double sampleToPieces = samplesToTimestampPieces(1, sampleRate);
    uint32_t eventIdx=0, sampleIdx=0, curTimestamp=0, frames=0, maxTimestamp=allEvents[eventsNum-1]->timestamp;
    double fCurTimestamp=0, fframes=0;

    int16_t* wavBuffer = malloc(BUFFER_FRAMES*2*sizeof(int16_t));
    if (!wavBuffer) {
        if (noteOffEvents) free(noteOffEvents);
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
                exportProgramNoteOnFromTrack(nt->key, nt->velocity*0.007874*tr->velocity, nt->track);
            } else {
                exportProgramNoteOffFromTrack(nt->key, nt->track);
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
        threadEditProcessPercentage(floatClip(jobOffset+jobPercentage*(fCurTimestamp/maxTimestamp), 0.0, 1.0));
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


static uint32_t simulateAudioForMP3(FILE* fptr, uint32_t sampleRate, double startDelay, double endDelay, float jobPercentage, float jobOffset) {
    uint32_t notesNum=0, eventsNum=0;
    Note noteOffEvents=NULL;
    Note* allEvents = _generateEventArray(&notesNum, &eventsNum, &noteOffEvents);
    if (!allEvents) return 0;

    lame_global_flags *gfp = lame_init();
    if (!gfp) {
        if (noteOffEvents) free(noteOffEvents);
        free(allEvents);
        return 0;
    }

    lame_set_in_samplerate(gfp, 44100);
    lame_set_num_channels(gfp, 2);
    lame_set_brate(gfp, 128);
    lame_set_quality(gfp, 2);

    if (lame_init_params(gfp)<0) {
        fprintf(stderr, "Failed to initialize LAME parameters\n");
        if (noteOffEvents) free(noteOffEvents);
        free(allEvents);
        return 0;
    }

    double pieceToSample = timestampPiecesToSamples(1, sampleRate);
    double sampleToPieces = samplesToTimestampPieces(1, sampleRate);
    uint32_t eventIdx=0, sampleIdx=0, curTimestamp=0, frames=0, maxTimestamp=allEvents[eventsNum-1]->timestamp;
    double fCurTimestamp=0, fframes=0;

    int16_t* wavBuffer = malloc(BUFFER_FRAMES*2*sizeof(int16_t));
    if (!wavBuffer) {
        lame_close(gfp);
        if (noteOffEvents) free(noteOffEvents);
        free(allEvents);
        return 0;
    }

    int mp3BufSize = (int)(1.25 * BUFFER_FRAMES + 7200);
    unsigned char* mp3Buffer = malloc(mp3BufSize);
    if (!mp3Buffer) {
        free(wavBuffer);
        lame_close(gfp);
        if (noteOffEvents) free(noteOffEvents);
        free(allEvents);
        return 0;
    }
    exportSynthPanic();


    #define ENCODE_AND_WRITE(num_frames) do { \
        int bytesWritten = lame_encode_buffer_interleaved(gfp, wavBuffer, (int)(num_frames), mp3Buffer, mp3BufSize); \
        if (bytesWritten>0) fwrite(mp3Buffer, 1, bytesWritten, fptr); \
    } while(0)


    if (startDelay<0) startDelay=5.0*BUFFER_FRAMES/(double)sampleRate;
    if (endDelay<0) endDelay=10.0*BUFFER_FRAMES/(double)sampleRate;
    
    uint32_t startingSamples = (uint32_t)(startDelay*sampleRate);
    uint32_t samples=startingSamples;
    while (startingSamples>0) {
        if (startingSamples>=BUFFER_FRAMES) {
            renderExportAudio(wavBuffer, BUFFER_FRAMES);
            ENCODE_AND_WRITE(BUFFER_FRAMES);
            startingSamples-=BUFFER_FRAMES;
        } else {
            renderExportAudio(wavBuffer, startingSamples);
            ENCODE_AND_WRITE(startingSamples);
            startingSamples=0;
        }
    }

    while (eventIdx<eventsNum) {
        while (eventIdx<eventsNum && allEvents[eventIdx]->timestamp<=curTimestamp) {
            Note nt = allEvents[eventIdx++];
            Track tr = globalProject->tracks+nt->track;
            if (nt>=noteOffEvents+notesNum || nt<noteOffEvents) {
                exportProgramNoteOnFromTrack(nt->key, nt->velocity*0.007874*tr->velocity, nt->track);
            } else {
                exportProgramNoteOffFromTrack(nt->key, nt->track);
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
        ENCODE_AND_WRITE(frames);
        threadEditProcessPercentage(floatClip(jobOffset+jobPercentage*(fCurTimestamp/maxTimestamp), 0.0, 1.0));
    }

    exportSynthPanic();

    uint32_t endingSamples = (uint32_t)(endDelay*sampleRate);
    while (endingSamples>0) {
        if (endingSamples>=BUFFER_FRAMES) {
            renderExportAudio(wavBuffer, BUFFER_FRAMES);
            ENCODE_AND_WRITE(BUFFER_FRAMES);
            endingSamples-=BUFFER_FRAMES;
        } else {
            renderExportAudio(wavBuffer, endingSamples);
            ENCODE_AND_WRITE(endingSamples);
            endingSamples=0;
        }
    }

    int finalBytes = lame_encode_flush(gfp, mp3Buffer, mp3BufSize);
    if (finalBytes > 0) fwrite(mp3Buffer, 1, finalBytes, fptr);
    #undef ENCODE_AND_WRITE

    free(wavBuffer);
    free(noteOffEvents);
    free(allEvents);
    free(mp3Buffer);
    lame_close(gfp);

    return samples;
}


int exportProjectAsWave(const char* filename) {
    if (!filename) return 1;

    FILE* fptr = fopen(filename, "wb");
    if (!fptr) return 1;

    threadEditProcessPercentage(0);
    threadEditProcessDescription(TextFormat("Exporting WAVE to %s...", GetFileName(filename)));

    uint32_t sampleRate = 44100;
    writeWavHeader(fptr, sampleRate);
    uint32_t samples = simulateAudio(fptr, sampleRate, 1.0, 0.0);
    updateWavHeader(fptr, samples);

    fclose(fptr);
    return (samples==0);
}


int exportProjectAsWaveForExportAll(const char* filename, int totalJobs, int currentJob) {
    if (!filename) return 1;

    FILE* fptr = fopen(filename, "wb");
    if (!fptr) return 1;

    uint32_t sampleRate = 44100;
    writeWavHeader(fptr, sampleRate);
    uint32_t samples = simulateAudio(fptr, sampleRate, 1.0/totalJobs, currentJob/(float)totalJobs);
    updateWavHeader(fptr, samples);

    fclose(fptr);
    return (samples==0);
}


int exportProjectAsMP3(const char* filename) {
    if (!filename) return 1;

    FILE* fptr = fopen(filename, "wb");
    if (!fptr) return 1;

    threadEditProcessPercentage(0);
    threadEditProcessDescription(TextFormat("Exporting MP3 to %s...", GetFileName(filename)));
    uint32_t samples = simulateAudioForMP3(fptr, 44100, -1, -1, 1.0, 0.0);

    fclose(fptr);
    return (samples==0);
}

int exportProjectAsMP3ForExportAll(const char* filename, int totalJobs, int currentJob) {
    if (!filename) return 1;

    FILE* fptr = fopen(filename, "wb");
    if (!fptr) return 1;

    uint32_t samples = simulateAudioForMP3(fptr, 44100, -1,-1, 1.0/totalJobs, currentJob/(float)totalJobs);

    fflush(fptr);
    fclose(fptr);
    return (samples==0);
}

int exportProjectAsMP3Extra(const char* filename, double startDelay, double endDelay, int totalJobs, int currentJob) {
    if (!filename) return 1;

    FILE* fptr = fopen(filename, "wb");
    if (!fptr) return 1;

    uint32_t samples = simulateAudioForMP3(fptr, 44100, startDelay, endDelay, 1.0/totalJobs, currentJob/(float)totalJobs);

    fflush(fptr);
    fclose(fptr);
    return (samples==0);
}