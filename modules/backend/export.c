#include <backend.h>
#include "backend_internal.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


int getTrackThemeColorIdx(int i);                       // modules/interface/render/tracks.c
void createTrackUIsFromScratch(uint8_t* colArr);        // modules/interface/render/tracks.c
void freeTrackUIs();                                    // modules/interface/render/tracks.c
void projectLoadTmpProject(ProjectData newProject);     // modules/backend/project.c


static void writeUint32(FILE* fptr, uint32_t data) {
    for (int i=0; i<4; i++) {
        int t = ((data >> (i<<3)) & 0xFF);
        fputc(t, fptr);
    }
}

static void writeUint16(FILE* fptr, uint16_t data) {
    for (int i=0; i<2; i++) {
        int t = ((data >> (i<<3)) & 0xFF);
        fputc(t, fptr);
    }
}

inline static void writeUint8(FILE* fptr, uint8_t data) {
    fputc((int)data, fptr);
}

static int readUint32(FILE* fptr, uint32_t* ret) {
    uint32_t tmp = 0;
    for (int i=0; i<4; i++) {
        int c = fgetc(fptr);
        if (c==EOF) return 1;
        tmp |= (((uint32_t)c)<<(i<<3));
    }
    *ret = tmp;
    return 0;
}

static int readUint16(FILE* fptr, uint16_t* ret) {
    uint16_t tmp = 0;
    for (int i=0; i<2; i++) {
        int c = fgetc(fptr);
        if (c==EOF) return 1;
        tmp |= (((uint16_t)c)<<(i<<3));
    }
    *ret = tmp;
    return 0;
}

static int readUint8(FILE* fptr, uint8_t* ret) {
    int c = fgetc(fptr);
    if (c==EOF) return 1;
    *ret = (uint8_t)c;
    return 0;
}

static int readString(FILE* fptr, char* buffer, int len) {
    for (int i=0; i<len; i++) {
        int c = fgetc(fptr);
        if (c==EOF) return 1;
        buffer[i] = (uint8_t)c;
    }
    buffer[len]=0;
    return 0;
}

static void writeString(FILE* fptr, const char* string) {
    const char* ch = string;
    while (*ch) {
        writeUint8(fptr, (uint8_t)(*ch));
        ch++;
    }
}

static int writeProjectHeader(FILE* fptr) {
    /*
    Project Header:
    - u16:      titleLength (without null byte)
    - u8[]:     title (without null byte)
    - u16:      tempo
    - u8:       time signature numerator
    - u8:       time signature denominator
    - u16:      number of tracks
    */

    uint16_t titleLength = (uint16_t)(strlen(globalProject->title));
    writeUint16(fptr, titleLength);
    writeString(fptr, globalProject->title);
    writeUint16(fptr, globalProject->tempo);
    writeUint8(fptr, globalProject->timeSignature.numerator);
    writeUint8(fptr, globalProject->timeSignature.denominator);
    writeUint32(fptr, globalProject->tracksNum);

    return 0;
}

static int readProjectHeader(FILE* fptr, ProjectData toLoad) {
    int ret=0;

    uint16_t titleLength = 0;
    ret += readUint16(fptr, &titleLength);
    
    char* title = malloc((titleLength+1)*sizeof(char));
    ret += readString(fptr, title, titleLength);

    uint16_t tempo = 0;
    ret += readUint16(fptr, &tempo);

    uint8_t tsNumerator=0, tsDenominator=0;
    ret += readUint8(fptr, &tsNumerator);
    ret += readUint8(fptr, &tsDenominator);

    uint32_t numOfTracks=0;
    ret += readUint32(fptr, &numOfTracks);


    if (!ret) {
        //fprintf(stdout, "Parsing Project Header:\n- titleLength: %u\n- title: %s\n- tempo: %u\n- timeSignature: %u/%u\n- numOfTracks: %u\n", titleLength, title, tempo, tsNumerator, tsDenominator, numOfTracks);

        toLoad->title = title;
        toLoad->tempo = tempo;
        toLoad->timeSignature.numerator = tsNumerator;
        toLoad->timeSignature.denominator = tsDenominator;
        toLoad->tracksNum = numOfTracks;
        toLoad->tracks = calloc(numOfTracks, sizeof(struct track_data));
    }

    return ret;
}


static uint32_t getUI32fromNormalizedFloat(float f) {
    if (f<=0) return 0;
    if (f>=1) return UINT32_MAX;
    return (uint32_t)(f*UINT32_MAX);
}

inline static float getNormalizedFloatFromUI32(uint32_t u) {
    return u/(float)UINT32_MAX;
}

static int writeTrackHeader(FILE* fptr, Track track, int trackIdx) {
    /*
    Track Header:
    - u8:       titleLength (without null byte)
    - u8[]:     title (without null byte)
    - u32:      volume (normalized u32 from float)
    - u32:      panning (normalized u32 from float)
    - u8:       program
    - u8:       channel
    - u8:       sustain
    - u8:       color
    - u32:      number of notes
    */

    if (!track) return 1;

    uint8_t titleLength = (uint8_t)(strlen(track->title));
    writeUint8(fptr, titleLength);
    writeString(fptr, track->title);
    writeUint32(fptr, getUI32fromNormalizedFloat(track->velocity));
    writeUint32(fptr, getUI32fromNormalizedFloat(track->panning));
    writeUint8(fptr, track->program);
    writeUint8(fptr, track->channel);
    writeUint8(fptr, track->sustain);
    
    int themeIdx = getTrackThemeColorIdx(trackIdx);
    if (themeIdx<0) themeIdx=0;
    writeUint8(fptr, (uint8_t)themeIdx);
    writeUint32(fptr, track->numElements);

    return 0;
}



static int writeNote(FILE* fptr, Note note) {
    /*
    Note Data:
    - u8:       key
    - u8:       velocity
    - u8:       channel
    - u8:       track
    - u32:      timestamp
    - u32:      duration
    */

    if (!note) return 1;

    writeUint8(fptr, note->key);
    writeUint8(fptr, note->velocity);
    writeUint8(fptr, note->channel);
    writeUint8(fptr, note->track);
    writeUint32(fptr, note->timestamp);
    writeUint32(fptr, note->duration);

    return 0;
}

static int readNote(FILE* fptr, Note* note) {
    *note = malloc(sizeof(struct note_data));
    if (!note) return 1;

    int ret=0;
    ret += readUint8(fptr, &((*note)->key));
    ret += readUint8(fptr, &((*note)->velocity));
    ret += readUint8(fptr, &((*note)->channel));
    ret += readUint8(fptr, &((*note)->track));
    ret += readUint32(fptr, &((*note)->timestamp));
    ret += readUint32(fptr, &((*note)->duration));

    return ret;
}

static int readTrack(FILE* fptr, Track track, uint8_t* col) {
    int ret=0;

    uint8_t titleLength=0;
    ret += readUint8(fptr, &titleLength);

    char* title = malloc((titleLength+1)*sizeof(char));
    ret += readString(fptr, title, titleLength);

    uint32_t u32volume=0, u32panning=0;
    float fvolume=0, fpanning=0;
    ret += readUint32(fptr, &u32volume);
    ret += readUint32(fptr, &u32panning);

    fvolume = getNormalizedFloatFromUI32(u32volume);
    fpanning = getNormalizedFloatFromUI32(u32panning);

    uint8_t program=0, channel=0, sustain=0, color=0;
    ret += readUint8(fptr, &program);
    ret += readUint8(fptr, &channel);
    ret += readUint8(fptr, &sustain);
    ret += readUint8(fptr, &color);

    uint32_t numberOfNotes=0;
    ret += readUint32(fptr, &numberOfNotes);

    if (!ret) {
        //fprintf(stdout, "Parsing Track:\n- titleLength: %u\n- title: %s\n- program: %u\n- fvolume: %f\n- fpanning: %f\n- color: %u\n- numOfNotes: %u\n", titleLength, title, program, fvolume, fpanning, color, numberOfNotes);
        *col = color;

        track->title = title;
        track->program = program;
        track->channel = channel;
        track->sustain = sustain;
        track->velocity = fvolume;
        track->panning = fpanning;

        track->capacity = numberOfNotes;
        track->numElements = numberOfNotes;

        track->notes = calloc(track->capacity, sizeof(Note));

        for (uint32_t i=0; i<numberOfNotes; i++) {
            Note note = NULL;
            ret += readNote(fptr, &note);
            (track->notes)[i] = note;
            if (ret) return ret;
        }
    }

    return ret;
}

static int writeTrack(FILE* fptr, Track track, int trackIdx) {
    if (!fptr || !track) return 1;

    int ret = writeTrackHeader(fptr, track, trackIdx);
    uint32_t notes = track->numElements;
    for (uint32_t i=0; i<notes; i++) ret += writeNote(fptr, (track->notes)[i]);
    
    return ret;
}

static int writeProject(FILE* fptr) {
    int ret = writeProjectHeader(fptr);
    uint16_t tracks = globalProject->tracksNum;
    for (uint16_t i=0; i<tracks; i++) ret+=writeTrack(fptr, globalProject->tracks+i, i);
    return ret;
}


static void freeInvalidProjectHeader(ProjectData prd) {
    if (!prd) return;
    
    if (prd->title) free(prd->title);
    prd->title = NULL;
    
    if (prd->tracks) {
        free(prd->tracks);
        prd->tracks = NULL;
        prd->tracksNum = 0;
    }
}

static void freeInvalidProjectTrack(Track track) {
    if (!track) return;

    if (track->notes) {
        uint32_t n = track->numElements;
        for (uint32_t i=0; i<n; i++) {
            if ((track->notes)[i]) {
                free((track->notes)[i]);
                (track->notes)[i] = NULL;
            }
        }
        free(track->notes);
    }
}


static void replaceCurrentProject(struct general_project_data newProject, uint8_t* colArr) {
    freeTrackUIs();
    projectLoadTmpProject(&newProject);
    createTrackUIsFromScratch(colArr);
}


static int loadProject(FILE* fptr) {
    struct general_project_data newProject={NULL,};


    int ret = 0;
    if (ret+=readProjectHeader(fptr, &newProject)) {
        freeInvalidProjectHeader(&newProject);
        //fprintf(stderr, "File Parsing Failed: Project Header\n");
        return ret;
    }

    //printf("newProject: timeSignature: %u/%u, tempo: %u\n", newProject.timeSignature.numerator, newProject.timeSignature.denominator, newProject.tempo);

    int tracks = newProject.tracksNum;
    uint8_t* colArr = calloc(tracks, sizeof(uint8_t));
    if (!colArr) {
        freeInvalidProjectHeader(&newProject);
        return ret;
    }

    for (int i=0; i<tracks; i++) {
        if (ret+=readTrack(fptr, newProject.tracks+i, colArr+i)) {
            for (int j=0; j<=i; j++) freeInvalidProjectTrack(newProject.tracks+j);

            fprintf(stderr, "File Parsing Failed: Project Track #%d\n", i);
            freeInvalidProjectHeader(&newProject);
            free(colArr);
            return ret;
        }
    }


    replaceCurrentProject(newProject, colArr);
    free(colArr);

    return 0;
}



int exportProjectTo(const char* filename) {
    FILE* fptr = fopen(filename, "wb");
    if (!filename) return 1;    // Couldn't open file

    int ret = writeProject(fptr);

    fclose(fptr);
    return ret;
}


int importProjectFrom(const char* filename) {
    FILE* fptr = fopen(filename, "rb");
    if (!filename) return 1;    // Couldn't open file

    int ret = loadProject(fptr);

    fclose(fptr);
    return ret;
}