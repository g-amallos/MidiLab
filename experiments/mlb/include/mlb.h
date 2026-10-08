#ifndef MLB_H
#define MLB_H

#include <stdint.h>


struct time_signature {
    uint8_t numerator;
    uint8_t denominator;
};

typedef struct note_data {
    uint8_t key;
    uint8_t velocity;
    uint8_t channel;
    uint8_t track;
    uint32_t timestamp;     // Relative
    uint32_t duration;      // Relative
    double ftimestamp;
    double fduration;
} *Note;


typedef struct track_data {
    char* title;
    uint8_t program;
    uint8_t channel;
    uint8_t sustain;
    uint8_t themeIdx;
    
    float velocity;
    float panning;

    uint8_t keyMin;
    uint8_t keyMax;
    uint32_t timestampStart;
    uint32_t timestampEnd;

    uint32_t capacity;      // Allocating more than needed, for fewer realloc calls
    uint32_t numElements;      // Number of actual saved note data (the first n in the array)
    Note* notes;

} *Track;


typedef struct general_project_data {
    char* title;
    uint16_t tempo;
    struct time_signature timeSignature;
    uint16_t tracksNum;
    Track tracks;
} *ProjectData;


extern struct general_project_data* globalProject;





int exportProjectTo(const char* filename);
int importProjectFrom(const char* filename);
uint32_t estimateFileSizeForProject();
int importTrackFrom(const char* filename, int idx);
int exportTrackTo(const char* filename, int idx);
int freeGlobalProject();


#endif