#ifndef CHORDS_H
#define CHORDS_H

#include <stdint.h>



enum chord_type {
    CHORD_MAJOR,
    CHORD_MINOR,
    CHORD_DIMINISHED,
    CHORD_AUGMENTED,
    CHORD_POWER,
    CHORD_SUSPENDED_2,
    CHORD_SUSPENDED_4,
    CHORD_DOMINANT_7,
    CHORD_MAJOR_7,
    CHORD_MINOR_7,
    CHORD_HALF_DIMINISHED,
    CHORD_FULLY_DIMINISHED,




    CHORD_END
};


struct chord {
    uint8_t found;
    uint8_t base;
    enum chord_type type;
    float score;
};




float getScoreFor(float arr[12], uint8_t base, enum chord_type type);
struct chord getChordFromIntensityArray(float arr[12]);
const char* getChordTypeString(enum chord_type type);
char* getChordFullName(enum chord_type type, uint8_t base);
char* getChordCompactName(enum chord_type type, uint8_t base);


#endif