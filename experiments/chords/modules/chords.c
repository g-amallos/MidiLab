#include <math.h>
#include <stdlib.h>
#include <chords.h>
#include <string.h>
#include <stdio.h>

#define CHORD_MAX_NOTES 4



struct chord_includes {
    uint8_t num;
    uint8_t notes[CHORD_MAX_NOTES];
};





struct chord_signature {
    enum chord_type type;
    struct chord_includes notes;
    const char* name;
};



static struct chord_signature chordSignatures[CHORD_END] = {
    [CHORD_MAJOR] = {
        .type = CHORD_MAJOR,
        .notes = {
            .num = 3,
            .notes = {0,4,7}
        },
        .name = "Major"
    },

    [CHORD_MINOR] = {
        .type = CHORD_MINOR,
        .notes = {
            .num = 3,
            .notes = {0,3,7}
        },
        .name = "Minor"
    },

    [CHORD_DIMINISHED] = {
        .type = CHORD_DIMINISHED,
        .notes = {
            .num = 3,
            .notes = {0,3,6}
        },
        .name = "Diminished"
    },

    [CHORD_AUGMENTED] = {
        .type = CHORD_AUGMENTED,
        .notes = {
            .num = 3,
            .notes = {0,4,8}
        },
        .name = "Augmented"
    },

    [CHORD_POWER] = {
        .type = CHORD_POWER,
        .notes = {
            .num = 2,
            .notes = {0,7}
        },
        .name = "Power"
    },

    [CHORD_SUSPENDED_2] = {
        .type = CHORD_SUSPENDED_2,
        .notes = {
            .num = 3,
            .notes = {0,2,7}
        },
        .name = "Suspended 2"
    },

    [CHORD_SUSPENDED_4] = {
        .type = CHORD_SUSPENDED_4,
        .notes = {
            .num = 3,
            .notes = {0,5,7}
        },
        .name = "Suspended 4"
    },

    [CHORD_DOMINANT_7] = {
        .type = CHORD_DOMINANT_7,
        .notes = {
            .num = 4,
            .notes = {0,4,7,10}
        },
        .name = "Dominant 7"
    },

    [CHORD_MAJOR_7] = {
        .type = CHORD_MAJOR_7,
        .notes = {
            .num = 4,
            .notes = {0,4,7,11}
        },
        .name = "Major7"
    },

    [CHORD_MINOR_7] = {
        .type = CHORD_MINOR_7,
        .notes = {
            .num = 4,
            .notes = {0,3,7,10}
        },
        .name = "Minor7"
    },

    [CHORD_HALF_DIMINISHED] = {
        .type = CHORD_HALF_DIMINISHED,
        .notes = {
            .num = 4,
            .notes = {0,3,6,10}
        },
        .name = "Half Diminished"
    },

    [CHORD_FULLY_DIMINISHED] = {
        .type = CHORD_FULLY_DIMINISHED,
        .notes = {
            .num = 4,
            .notes = {0,3,6,9}
        },
        .name = "Fully Diminished"
    },
};

static const char* scaleStrings1[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
//static const char* scaleStrings2[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
static const char* chordTypeStrings[CHORD_END] = {
    [CHORD_MAJOR] = "",
    [CHORD_MINOR] = "m",
    [CHORD_DIMINISHED] = "dim",
    [CHORD_AUGMENTED] = "aug",
    [CHORD_POWER] = "5",
    [CHORD_SUSPENDED_2] = "sus2",
    [CHORD_SUSPENDED_4] = "sus4",
    [CHORD_DOMINANT_7] = "7",
    [CHORD_MAJOR_7] = "maj7",
    [CHORD_MINOR_7] = "m7",
    [CHORD_HALF_DIMINISHED] = "m7b5",
    [CHORD_FULLY_DIMINISHED] = "dim7",
};



float getScoreFor(float arr[12], uint8_t base, enum chord_type type) {
    float numerator=0.0, denominator=0.0;
    float d1=0.0, d2=0.0;
    float target[12] = {0,};

    for (int i=0; i<chordSignatures[type].notes.num; i++) target[chordSignatures[type].notes.notes[i]] = 0.9;
    target[chordSignatures[type].notes.notes[0]] = 1.0;

    for (int i=0; i<12; i++) {
        if (target[i]<1e-3) target[i]=-0.35;

        numerator += arr[(i+base)%12]*target[i];
        d1 += arr[i]*arr[i];
        d2 += target[i]*target[i];
    }
    
    denominator = sqrtf(d1*d2);
    return numerator/denominator;
}

struct chord getChordFromIntensityArray(float arr[12]) {
    struct chord ret = {.found=0, .base=0, .score=0, .type=CHORD_END};
    if (!arr) return ret;

    float maxScore=-1.0;
    enum chord_type maxScType=CHORD_MAJOR;
    uint8_t maxScBase=0;

    for (uint8_t base=0; base<12; base++) {
        for (enum chord_type type=CHORD_MAJOR; type<CHORD_END; type++) {
            float score = getScoreFor(arr, base, type);
            //char* str = getChordFullName(type, base);
            //printf("[Intensity Array] score: %.2f%% for chord: %s\n", score*100.0, str);
            //free(str);
            if (score>maxScore) {
                maxScore = score;
                maxScBase = base;
                maxScType = type;
            }
        }
    }

    ret.base = maxScBase;
    ret.found = (maxScore>0.8);
    ret.score = maxScore;
    ret.type = maxScType;
    return ret;
}


const char* getChordTypeString(enum chord_type type) {
    if (type<0 || type>=CHORD_END) return NULL;
    return chordSignatures[type].name;
}

char* getChordFullName(enum chord_type type, uint8_t base) {
    if (type<0 || type>=CHORD_END) return NULL;
    base = base%12;

    const char* baseStr[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const char* str1=baseStr[base];
    const char* str2=chordSignatures[type].name;
    if (!str1 || !str2) return NULL;

    int l1=strlen(str1), l2=strlen(str2);
    int cap=l1+l2+2;

    char* str = malloc(sizeof(char)*cap);
    if (!str) return NULL;
    memcpy(str, str1, sizeof(char)*l1);
    memcpy(str+l1+1, str2, sizeof(char)*l2);
    str[l1]=' ';
    str[cap-1]=0;

    return str;
}

char* getChordCompactName(enum chord_type type, uint8_t base) {
    if (type<0 || type>=CHORD_END) return NULL;
    base = base%12;

    const char* str1 = scaleStrings1[base];
    const char* str2 = chordTypeStrings[type];

    int l1=strlen(str1), l2=strlen(str2);
    int cap=l1+l2+1;

    char* str = malloc(sizeof(char)*cap);
    if (!str) return NULL;

    memcpy(str, str1, sizeof(char)*l1);
    memcpy(str+l1, str2, sizeof(char)*l2);
    str[cap-1]=0;
    return str;
}