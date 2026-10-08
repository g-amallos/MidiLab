#include "tiles.h"
#include <math.h>
#include <stdlib.h>
#include <utils.h>
#include <string.h>
#include <stdio.h>

#define CHORD_MAX_NOTES 4



struct chord_includes {
    uint8_t num;
    uint8_t notes[CHORD_MAX_NOTES];
};



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

struct chord_signature {
    enum chord_type type;
    struct chord_includes notes;
};



static struct chord_signature chordSignatures[CHORD_END] = {
    [CHORD_MAJOR] = {
        .type = CHORD_MAJOR,
        .notes = {
            .num = 3,
            .notes = {0,4,7}
        }
    },

    [CHORD_MINOR] = {
        .type = CHORD_MINOR,
        .notes = {
            .num = 3,
            .notes = {0,3,7}
        }
    },

    [CHORD_DIMINISHED] = {
        .type = CHORD_DIMINISHED,
        .notes = {
            .num = 3,
            .notes = {0,3,6}
        }
    },

    [CHORD_AUGMENTED] = {
        .type = CHORD_AUGMENTED,
        .notes = {
            .num = 3,
            .notes = {0,4,8}
        }
    },

    [CHORD_POWER] = {
        .type = CHORD_POWER,
        .notes = {
            .num = 2,
            .notes = {0,7}
        }
    },

    [CHORD_SUSPENDED_2] = {
        .type = CHORD_SUSPENDED_2,
        .notes = {
            .num = 3,
            .notes = {0,2,7}
        }
    },

    [CHORD_SUSPENDED_4] = {
        .type = CHORD_SUSPENDED_4,
        .notes = {
            .num = 3,
            .notes = {0,5,7}
        }
    },

    [CHORD_DOMINANT_7] = {
        .type = CHORD_DOMINANT_7,
        .notes = {
            .num = 4,
            .notes = {0,4,7,10}
        }
    },

    [CHORD_MAJOR_7] = {
        .type = CHORD_MAJOR_7,
        .notes = {
            .num = 4,
            .notes = {0,4,7,11}
        }
    },

    [CHORD_MINOR_7] = {
        .type = CHORD_MINOR_7,
        .notes = {
            .num = 4,
            .notes = {0,3,7,10}
        }
    },

    [CHORD_HALF_DIMINISHED] = {
        .type = CHORD_HALF_DIMINISHED,
        .notes = {
            .num = 4,
            .notes = {0,3,6,10}
        }
    },

    [CHORD_FULLY_DIMINISHED] = {
        .type = CHORD_FULLY_DIMINISHED,
        .notes = {
            .num = 4,
            .notes = {0,3,6,9}
        }
    },
};

//static const char* scaleStrings[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char* scaleStrings[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
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



struct chord {
    uint8_t found;
    uint8_t base;
    enum chord_type type;
    float score;
};

static float getScoreFor(float arr[12], uint8_t base, enum chord_type type) {
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
    if (denominator==0.0) return 0.0;
    return numerator/denominator;
}


static struct chord getChordFromIntensityArray(float arr[12]) {
    struct chord ret = {.found=0, .base=0, .score=0, .type=CHORD_END};
    if (!arr) return ret;

    float maxScore=-1.0;
    enum chord_type maxScType=CHORD_END;
    uint8_t maxScBase=0;

    for (uint8_t base=0; base<12; base++) {
        for (enum chord_type type=CHORD_MAJOR; type<CHORD_END; type++) {
            float score = getScoreFor(arr, base, type);
            if (score>maxScore) {
                maxScore = score;
                maxScBase = base;
                maxScType = type;
            }
        }
    }

    ret.base = maxScBase;
    ret.found = (maxScore>0.65);
    ret.score = maxScore;
    ret.type = maxScType;
    return ret;
}


static char* getChordCompactName(enum chord_type type, uint8_t base) {
    if (type<0 || type>=CHORD_END) return NULL;
    base = base%12;

    const char* str1 = scaleStrings[base];
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

static struct chord_render_info getChordStringDebug(struct chord chord) {
    struct chord_render_info ret = {NULL, 0.0, 0};
    if (chord.score>0) ret.string = getChordCompactName(chord.type, chord.base);
    if (!(ret.string)) return ret;
    ret.score = chord.score;
    ret.found = chord.found;
    return ret;
}


static float getNoteIntensity(Note note, float trackVolume, uint32_t start, uint32_t end) {
    if (note->timestamp>=end) return 0.0;
    if (note->duration+note->timestamp<=start) return 0.0;
    uint32_t st = uint32Clip(note->timestamp, start, end);
    uint32_t en = uint32Clip(note->timestamp+note->duration, start, end);
    return trackVolume*(en-st)*(note->velocity/127.0)/(end-start);
}


void setupChords() {
    double dur = settings.info.duration;
    int measures = (int)ceil(dur/(beatDuration*settings.beatsInMeasure));

    settings.chords.num = 0;
    settings.chords.chords = NULL;

    if (measures<=0) return;
    struct chord_render_info* ret = malloc(sizeof(struct chord_render_info)*measures);
    if (!ret) return;

    settings.chords.num = measures;
    settings.chords.chords = ret;

    uint32_t startingIdx=0, n=settings.notes.size, nextMeasureIdxFound=0, nextMeasureIdx=-1, measureDurTmst=trackPiecesInBeat()*settings.beatsInMeasure, measure=0;
    uint32_t curMeasureTmst=0, nextMeasureTmst=measureDurTmst;
    Note* ntArr = settings.notes.notes; 
    Note nt=NULL;

    while (startingIdx<n) {
        float intensity[12]={0,};

        while (1) {
            if (startingIdx>=n) break;
            nt = ntArr[startingIdx];
            if (!nextMeasureIdxFound && nt->timestamp+nt->duration>nextMeasureTmst) {
                nextMeasureIdx=startingIdx;
                nextMeasureIdxFound=1;
            }
            if (nt->timestamp>=nextMeasureTmst) break;
            Track track = trackGetAtIdx(nt->track);
            if (trackGetProgram(track)<128) {
                intensity[nt->key%12] += getNoteIntensity(nt, trackGetVelocity(track), curMeasureTmst, nextMeasureTmst);
            }
            startingIdx++;
        }
        if (!nextMeasureIdxFound) nextMeasureIdx=startingIdx;

        struct chord chord = getChordFromIntensityArray(intensity);
        ret[measure] = getChordStringDebug(chord);
        //if (ret[measure].score<0.2) printf("%d: score: %.5f\n", measure, ret[measure].score);

        curMeasureTmst=nextMeasureTmst;
        nextMeasureTmst+=measureDurTmst;
        startingIdx=nextMeasureIdx;
        nextMeasureIdx=-1;
        nextMeasureIdxFound=0;
        measure++;
    }
}