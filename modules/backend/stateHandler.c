#include "backend_internal.h"
#include <stdlib.h>
#include <string.h>
#include <synth.h>
#include <utils.h>
#include <stdio.h>
#include <math.h>



struct backend_state_handler _globalHandler = {
    .time = {
        .time = 0,
        .ticks = 0,

        
        .tempo = 120,
        .ppqn = 480,
        .ticksPerBeat = 440,
        .measureDuration = 0,
        .beatDuration = 0,
        .visibleDuration = 10,

        .timeSignature = {
            .numerator = 4,
            .denominator = 4
        },

        .timeline = {
            .playing = 0,
            .timeShown = 1,
            .loopEnabled = 0,

            .timestamp = 0,
            .prePlayTimestamp = 0,
            .time = 0,
            .prePlayTime = 0,
            .loopStart = 0,
            .loopEnd = 0
        }
        
        
    },
    .keys = {
        .inputAllowed = 0,
        .type = T_KEYBOARD_NONE,
        .keys = {{0,0},}
    },
    .project = &_globalProject,
    .selectedTrack = -1,
    .selector = {
        .primary = {
            .keyMin = 255,
            .keyMax = 0,
            .clickHold = 0,
            .rollRect = {
                .topLeft = {.fkey=-1, .timestamp=0},
                .bottomRight = {.fkey=-1, .timestamp=0},
            },
            .timestampStart = 0,
            .timestampEnd = 0,
            .capacity = 0,
            .notesNum = 0,
            .notes = NULL
        },
        .secondary = {
            .keyMin = 255,
            .keyMax = 0,
            .clickHold = 0,
            .rollRect = {
                .topLeft = {.fkey=-1, .timestamp=0},
                .bottomRight = {.fkey=-1, .timestamp=0},
            },
            .timestampStart = 0,
            .timestampEnd = 0,
            .capacity = 0,
            .notesNum = 0,
            .notes = NULL
        }
    }
};



StateHandler globalStateHandler = &_globalHandler;


void globalStateHandlerInit() {
    globalStateHandler->time.ppqn = 480;    // Default
    globalStateHandler->time.beatDuration = 60.0/globalStateHandler->time.tempo;
    globalStateHandler->time.measureDuration = globalStateHandler->time.timeSignature.numerator*globalStateHandler->time.beatDuration;
    globalStateHandler->time.ticksPerBeat = (globalStateHandler->time.ppqn*4)/globalStateHandler->time.timeSignature.denominator;
}

void _globalHandlerUpdateDurations() {
    globalStateHandler->time.beatDuration = 60.0/globalStateHandler->time.tempo;
    globalStateHandler->time.measureDuration = globalStateHandler->time.timeSignature.numerator*globalStateHandler->time.beatDuration;
    globalStateHandler->time.ticksPerBeat = (globalStateHandler->time.ppqn*4)/globalStateHandler->time.timeSignature.denominator;
}

void _globalStateHandlerUpdateTempo(double tempo) {
    if (!globalStateHandler) return;
    globalStateHandler->time.tempo = tempo;
}

double globalHandlerGetVisibleDuration() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.visibleDuration;
}

void globalHandlerSetVisibleDuration(double duration) {
    if (!globalStateHandler) return;
    duration = floatClip(duration, 0.1, 180);
    double old = globalStateHandler->time.visibleDuration;
    globalStateHandler->time.visibleDuration = duration;

    globalStateHandler->time.time = doubleMax((globalStateHandler->time.time-globalStateHandler->time.timeline.time)/old*duration+globalStateHandler->time.timeline.time, 0);

}


void globalHandlerSetLineTime(double time) {
    if (!globalStateHandler) return;
    time = floatMax(time, 0);
    globalStateHandler->time.timeline.time = time;
    globalStateHandler->time.timeline.timestamp = (uint32_t)(trackPiecesInBeat()*time/globalStateHandler->time.beatDuration);
}


double timestampPiecesToSeconds(uint32_t pieces) {
    return pieces*globalStateHandler->time.beatDuration/trackPiecesInBeat();
}

double timestampPiecesToSamples(uint32_t pieces, uint32_t sampleRate) {
    return (sampleRate*(uint64_t)pieces)*globalStateHandler->time.beatDuration/trackPiecesInBeat();
}

double samplesToTimestampPieces(uint32_t samples, uint32_t sampleRate) {
    return (samples/(double)sampleRate)*trackPiecesInBeat()/globalStateHandler->time.beatDuration;
}

double globalHandlerGetTime() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.time;
}

void globalHandlerSetTime(double time) {
    if (!globalStateHandler) return;
    time = floatMax(time, 0);
    globalStateHandler->time.time = time;
}


double globalHandlerGetLineTime() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.timeline.time;
}

uint32_t globalHandlerGetLineTimestamp() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.timeline.timestamp;
}

double globalHandlerDurationToMeasures(double seconds) {
    if (!globalStateHandler) return 0;
    return seconds/globalStateHandler->time.measureDuration;
}

double globalHandlerMeasuresToDuration(double measures) {
    if (!globalStateHandler) return 0;
    return measures*globalStateHandler->time.measureDuration;
}

int globalHandlerGetBeatsInMeasure() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.timeSignature.numerator;
}

double globalHandlerGetMeasureDuration() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.measureDuration;
}

double globalHandlerGetBeatDuration() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.beatDuration;
}

int globalHandlerIsPlaying() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.timeline.playing;
}

int globalHandlerIsTimeLineShown() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.timeline.timeShown;
}

void globalHandlerPlay() {
    if (!globalStateHandler) return;
    globalStateHandler->time.timeline.playing = 1;
    globalStateHandler->time.timeline.prePlayTime = globalStateHandler->time.timeline.time;
    globalStateHandler->time.timeline.prePlayTimestamp = globalStateHandler->time.timeline.timestamp;
}

void globalHandlerPause() {
    if (!globalStateHandler) return;
    globalStateHandler->time.timeline.playing = 0;
    globalStateHandler->time.timeline.time = globalStateHandler->time.timeline.prePlayTime;
    globalStateHandler->time.timeline.timestamp = globalStateHandler->time.timeline.prePlayTimestamp;
}

void globalHandlerEnableLoop() {
    if (!globalStateHandler) return;
    globalStateHandler->time.timeline.loopEnabled = 1;
}

void globalHandlerDisableLoop() {
    if (!globalStateHandler) return;
    globalStateHandler->time.timeline.loopEnabled = 0;
}

int globalHandlerIsLoopEnabled() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.timeline.loopEnabled;
}

struct time_signature globalHandlerGetTimeSignature() {
    if (!globalStateHandler || !globalProject) return (struct time_signature){4,4};
    return globalStateHandler->time.timeSignature;
}

void globalHandlerSetTimeSignature(struct time_signature tsign) {
    if (!globalStateHandler || !globalProject) return;
    if (tsign.numerator>0 && tsign.numerator<7 && (tsign.denominator==1 || tsign.denominator==2 || tsign.denominator==4 || tsign.denominator==8)) {
        globalStateHandler->time.timeSignature = tsign;
        globalProject->timeSignature = tsign;
        _globalHandlerUpdateDurations();
        projectUpdateStateSomethingChanged();
    }
}

void globalHandlerSelectTrack(int idx) {
    if (idx<0 || !globalStateHandler || !globalProject || !(globalProject->tracks) || idx>=globalProject->tracksNum) {
        globalStateHandler->selectedTrack = -1;
        globalStateHandler->keys.type = T_KEYBOARD_NONE;
        globalStateHandler->keys.inputAllowed = 0;
        return;
    }
    globalStateHandler->selectedTrack = idx;
    if (globalStateHandler->keys.type!=T_KEYBOARD_HORIZONTAL && globalStateHandler->keys.type!=T_KEYBOARD_VERTICAL) globalStateHandler->keys.type=T_KEYBOARD_HORIZONTAL;
    globalStateHandler->keys.inputAllowed = 1;
    globalHandlerClearNotesSelected();
}

int globalHandlerGetSelectedTrack() {
    if (!globalStateHandler || !globalProject || !(globalProject->tracks)) return -1;
    return globalStateHandler->selectedTrack;
}

enum keyboard_render_types globalStateHandlerGetKeyboardType() {
    if (!globalStateHandler) return T_KEYBOARD_NONE;
    return globalStateHandler->keys.type;
}

void globalHandlerSetKeyboardType(enum keyboard_render_types view) {
    if (view!=T_KEYBOARD_NONE && view!=T_KEYBOARD_HORIZONTAL && view!=T_KEYBOARD_VERTICAL) return;
    globalHandlerClearNotesSelected();
    int trn=globalStateHandler->project->tracksNum, trs=globalStateHandler->selectedTrack;
    if ((trs>=0 && trs<trn) && (view==T_KEYBOARD_HORIZONTAL || view==T_KEYBOARD_VERTICAL)) globalStateHandler->keys.type=view;
    else {
        globalStateHandler->keys.type = T_KEYBOARD_NONE;
        globalStateHandler->selectedTrack = -1;
    }
}

void globalHandlerUpdateKey(int key, uint8_t velocity) {
    if (!globalStateHandler || !(globalStateHandler->keys.inputAllowed) || key<0 || key>=128) return;
    globalStateHandler->keys.keys[key].velocity = velocity;
}

void globalHandlerUpdateKeyAndPlaySynth(int key, uint8_t velocity) {
    if (!globalStateHandler || !(globalStateHandler->keys.inputAllowed) || key<0 || key>=128) return;
    globalStateHandler->keys.keys[key].velocity = velocity;
    //synthProgramNoteOn();
    struct track_data track = (globalStateHandler->project->tracks)[globalStateHandler->selectedTrack];
    synthProgramNoteOnPanning(key, 0.007874*velocity, track.program, track.panning);
}


struct array_indices {
    int foundElements;
    uint32_t start;     // Inclusive if foundElements
    uint32_t end;       // Inclusive if foundElements
};

static struct array_indices getArrayIndicesOfTrack(Track track, uint32_t start, uint32_t end) {
    struct array_indices ret = {0,0,0};
    if (!track || start>end || track->numElements<=0) return ret;


    uint32_t left=0;
    uint32_t right=track->numElements;  // Exclusive

    while (right>left) {
        uint32_t center = left+((right-left)>>1);
        if ((track->notes)[center]->timestamp<start) left=center+1;
        else right=center;
    }

    ret.start = left;
    right = track->numElements;  // Exclusive

    while (right>left) {
        uint32_t center = left+((right-left)>>1);
        if ((track->notes)[center]->timestamp>end) right=center;
        else left=center+1;
    }

    ret.end = right-1;
    if (right==0 || right<=ret.start) ret.foundElements=0;
    else ret.foundElements=1;
    
    return ret;


    /*
    uint32_t left=0, right=0;

    uint32_t startLeft=0, endRight=track->numElements-1;
    left=endRight;
    while (1) {
        if (track->notes[left]->timestamp<start) break;

        uint32_t tmp = (startLeft+left)>>1;
        if (track->notes[tmp]->timestamp>=start) left=tmp;
        else if (track->notes[tmp]->timestamp<start) startLeft=tmp;

        if (left==startLeft) break;
    }

    while (1) {
        if (track->notes[right]->timestamp>end) break;

        uint32_t tmp = (endRight+right)>>1;
        if (track->notes[tmp]->timestamp<=end) right=tmp;
        else if (track->notes[tmp]->timestamp>end) endRight=tmp;

        if (right==endRight) break;
    }

    ret.start = left;
    ret.end = right;
    if (left>right || track->notes[right]->timestamp>end || track->notes[left]->timestamp<start) ret.foundElements=0;
    else ret.foundElements = 1;

    return ret;*/
}


static void registerMidiEventsToActionsFrom(uint32_t timestampStart, uint32_t timestampEnd) {
    if (timestampEnd<=timestampStart) return;

    double now=GetTime();

    //fprintf(stderr, "`registerMidiEventsToActionsFrom`: Entered\n");

    uint16_t tracki = 0;
    for (; tracki<globalProject->tracksNum; tracki++) {
        //fprintf(stderr, "`registerMidiEventsToActionsFrom`: Loop #1 (%u)\n", tracki);
        struct array_indices indices = getArrayIndicesOfTrack(globalProject->tracks+tracki, timestampStart, timestampEnd); //NOT FINISHED! CONTINUE FROM THIS POINT
        
        if (indices.foundElements) {
            //fprintf(stderr, "`registerMidiEventsToActionsFrom`: Indices: Found=%d, start=%u, end=%u\n", indices.foundElements, indices.start, indices.end);
            Track track = globalProject->tracks+tracki;
            for (uint32_t i=indices.start; i<=indices.end; i++) {
                Note note = track->notes[i];
                note->channel = track->program;
                //printf("Time: %7.4lf | Note On: %u, Velocity=%u, Timestamp=%u | Start=%u, End=%u\n", now, note->key, note->velocity, note->timestamp, timestampStart, timestampEnd);
                
                synthProgramNoteOnPanning(note->key, 0.007874*note->velocity*track->velocity, track->program, track->panning);
                midiActionAdd(midiCreateEventForNoteOff(note), now+timestampPiecesToSeconds(note->duration));
            }
        }
    }
}



void globalHandlerUpdateTick() {

    float dt = GetFrameTime();

    if (globalStateHandler->time.timeline.playing) {
        struct backend_time_handler th = globalStateHandler->time;

        double oldT = th.timeline.time;
        double newT = (globalStateHandler->time.timeline.time+=dt);

        uint32_t oldTst = th.timeline.timestamp;
        uint32_t newTst = (uint32_t)(trackPiecesInBeat()*newT/globalStateHandler->time.beatDuration);

        globalStateHandler->time.timeline.timestamp = newTst;

        registerMidiEventsToActionsFrom(oldTst, newTst);
        th = globalStateHandler->time;
        globalStateHandler->time.time = doubleMax(0*oldT*newT, th.timeline.time-0.35*th.visibleDuration);

    } else {

    }
}


void globalHandlerMoveTimeDivAccordingToTimeLine(float startPadding, float endPadding) {
    struct backend_time_handler th = globalStateHandler->time;
    double tmp=th.time;
    if (tmp+startPadding*th.visibleDuration>th.timeline.time) tmp=th.timeline.time-startPadding*th.visibleDuration;
    if (tmp+(1.0-endPadding)*th.visibleDuration<th.timeline.time) tmp=th.timeline.time-(1.0-endPadding)*th.visibleDuration;
    globalStateHandler->time.time = doubleMax(0, tmp);
}

void globalHandlerSetToNextMeasure() {
    uint32_t piecesInMeasure = trackPiecesInBeat()*globalHandlerGetBeatsInMeasure();

    uint64_t oldTst = globalStateHandler->time.timeline.timestamp;
    uint64_t tmp = (oldTst/piecesInMeasure+1)*piecesInMeasure;

    if (tmp<UINT32_MAX) { // if (tmp<UINT32_MAX)
        globalStateHandler->time.timeline.timestamp = (uint32_t)tmp;
        globalStateHandler->time.timeline.time = timestampPiecesToSeconds(globalStateHandler->time.timeline.timestamp);
        globalHandlerMoveTimeDivAccordingToTimeLine(0, 0.15);
    }
}

void globalHandlerSetToPreviousMeasure() {
    uint32_t piecesInMeasure = trackPiecesInBeat()*globalHandlerGetBeatsInMeasure();

    uint64_t oldTst = globalStateHandler->time.timeline.timestamp;
    uint64_t tmp = (oldTst/piecesInMeasure)*piecesInMeasure;
    if (tmp==oldTst) tmp-=piecesInMeasure;

    if (tmp<UINT32_MAX && oldTst>tmp) {
        globalStateHandler->time.timeline.timestamp = (uint32_t)tmp;
        globalStateHandler->time.timeline.time = timestampPiecesToSeconds(globalStateHandler->time.timeline.timestamp);
        globalHandlerMoveTimeDivAccordingToTimeLine(0.15, 0);
    }
}

static void _freeNoteSelector() {
    if (globalStateHandler->selector.primary.notes) free(globalStateHandler->selector.primary.notes);
    if (globalStateHandler->selector.secondary.notes) free(globalStateHandler->selector.secondary.notes);

    globalStateHandler->selector = (struct notes_selector){
        .primary = {
            .keyMin = 255,
            .keyMax = 0,
            .clickHold = 0,
            .rollRect = {
                .topLeft = {.fkey=-1, .timestamp=0},
                .bottomRight = {.fkey=-1, .timestamp=0},
            },
            .timestampStart = 0,
            .timestampEnd = 0,
            .capacity = 0,
            .notesNum = 0,
            .notes = NULL
        },
        .secondary = {
            .keyMin = 255,
            .keyMax = 0,
            .clickHold = 0,
            .rollRect = {
                .topLeft = {.fkey=-1, .timestamp=0},
                .bottomRight = {.fkey=-1, .timestamp=0},
            },
            .timestampStart = 0,
            .timestampEnd = 0,
            .capacity = 0,
            .notesNum = 0,
            .notes = NULL
        }
    };
}

void globalHandlerClearNotesSelected() {
    if (!globalStateHandler) return;
    _freeNoteSelector();
}

static void _selectorVectorReset(struct notes_selection* selector) {
    if (!selector) return;
    if (selector->notes) free(selector->notes);
    *selector = (struct notes_selection){
        .keyMin = 255,
        .keyMax = 0,
        .clickHold = 0,
        .rollRect = {
            .topLeft = {.fkey=-1, .timestamp=0},
            .bottomRight = {.fkey=-1, .timestamp=0},
        },
        .timestampStart = 0,
        .timestampEnd = 0,
        .capacity = 0,
        .notesNum = 0,
        .notes = NULL
    };
}

static int _initSelectorVector(struct notes_selection* selector) {
    if (!selector) return 1;
    if (!(selector->notes)) {
        uint32_t cap = 32;
        selector->notes = calloc(cap, sizeof(Note));
        selector->capacity = cap*(!!(selector->notes));
        selector->notesNum = 0;
    }
    return !(selector->notes);
}

static int _selectorVectorDuplicateCapacity(struct notes_selection* selector) {
    uint32_t oldCap = selector->capacity;
    uint32_t newCap = (oldCap<<1);

    Note* arr = realloc(selector->notes, newCap*sizeof(Note));
    if (!arr) return 1;

    selector->capacity = newCap;
    selector->notes = arr;
    return 0;
}

static int _selectorVectorHalveCapacity(struct notes_selection* selector) {
    uint32_t oldCap = selector->capacity;
    uint32_t newCap = (oldCap>>1);
    if (newCap<32) newCap=32;

    Note* arr = realloc(selector->notes, newCap*sizeof(Note));
    if (!arr) return 1;

    selector->capacity = newCap;
    selector->notes = arr;
    return 0;
}

static int _intCompareFunc(const void* a, const void* b) {
    if (!a && !b) return 0;
    if (!a || !b) return a?-1:1;
    int na=*(int*)a, nb = *(int*)b;
    if (na>nb) return 1;
    if (na<nb) return -1;
    return 0;
}

static int _selectorVectorNoteCompare(const void* a, const void* b) {
    if (!a && !b) return 0;
    if (!a || !b) return a?-1:1;
    
    Note na = *(Note*)a;
    Note nb = *(Note*)b;

    if (na->timestamp>nb->timestamp) return 1;
    if (na->timestamp<nb->timestamp) return -1;
    return 0;
}

/*static*/ uint32_t _tracksGetTimestampIdx(Track track, uint32_t timestamp) {
    if (!track || !(track->notes)) return 0;
    uint32_t left=0, right=track->numElements;
    while (right>left) {
        uint32_t center = left+((right-left)>>1);
        if ((track->notes)[center]->timestamp<timestamp) left=center+1;
        else right=center;
    }
    return right;
}

static int _tracksVectorFindWhereNoteShouldBe(Track track, Note note) {
    if (!track || !(track->notes) || !note) return -1;
    uint32_t left=0, right=track->numElements;
    uint32_t target=note->timestamp;
    while (right>left) {
        uint32_t center = left+((right-left)>>1);
        if ((track->notes)[center]==note) return center;
        if ((track->notes)[center]->timestamp<target) left=center+1;
        else right=center;
    }
    uint32_t nidx=(uint32_t)right;
    while (nidx<track->numElements && note!=(track->notes)[nidx] && target<=(track->notes)[nidx]->timestamp) nidx++;
    if (nidx>=track->numElements || note!=(track->notes)[nidx]) return -1;
    else return (int)nidx;
}

static int _selectorVectorFindWhereNoteShouldBe(struct notes_selection* selector, Note note) {
    if (!selector || !(selector->notes) || !note) return -1;
    uint32_t left=0, right=selector->notesNum;
    uint32_t target=note->timestamp;
    while (right>left) {
        uint32_t center = left+((right-left)>>1);
        if ((selector->notes)[center]==note) return center;
        if ((selector->notes)[center]->timestamp<target) left=center+1;
        else right=center;
    }
    return right;
}

static int _selectorVectorFindNoteLinearlyAfter(struct notes_selection* selector, Note note, int idx) {
    if (idx<0) return idx;
    uint32_t target=note->timestamp, nidx=(uint32_t)idx;
    while (nidx<selector->notesNum && note!=(selector->notes)[nidx] && target<=(selector->notes)[nidx]->timestamp) nidx++;
    if (nidx>=selector->notesNum || note!=(selector->notes)[nidx]) return -1;
    else return (int)nidx;
}

static int _selectorVectorFind(struct notes_selection* selector, Note note) {
    int idx = _selectorVectorFindWhereNoteShouldBe(selector, note);
    return _selectorVectorFindNoteLinearlyAfter(selector, note, idx);
}

/*static*/ void _selectorVectorSort(struct notes_selection* selector) {
    if (!selector || !(selector->notes) || !(selector->notesNum)) return;
    qsort(selector->notes, selector->notesNum, sizeof(Note), _selectorVectorNoteCompare);
}

static void _selectorVectorUpdateValuesOnNoteAddition(struct notes_selection* selector, Note note) {
    if (!selector || !(selector->notes)) return;
    if (!(selector->notesNum)) {
        selector->keyMin = note->key;
        selector->keyMax = note->key;
        selector->timestampStart = note->timestamp;
        selector->timestampEnd = note->timestamp+note->duration;
    } else {
        if (selector->keyMin>note->key) selector->keyMin=note->key;
        if (selector->keyMax<note->key) selector->keyMax=note->key;
        if (selector->timestampStart>note->timestamp) selector->timestampStart=note->timestamp;
        if (selector->timestampEnd<note->timestamp+note->duration) selector->timestampEnd=note->timestamp+note->duration;
    }
}

static void _selectorVectorUpdateValues(struct notes_selection* selector) {
    if (!selector) return;
    if (!(selector->notes) || !(selector->notesNum)) {
        selector->keyMin=0;
        selector->keyMax=0;
        selector->timestampStart=0;
        selector->timestampEnd=0;
        return;
    }

    selector->timestampStart = (selector->notes)[0]->timestamp;
    uint8_t keyMin=128, keyMax=0;
    uint32_t timestampEnd=0, idx=0, num=selector->notesNum;
    for (; idx<num; idx++) {
        Note nt = (selector->notes)[idx];
        if (nt->timestamp+nt->duration>timestampEnd) timestampEnd=nt->timestamp+nt->duration;
        if (nt->key>keyMax) keyMax=nt->key;
        if (nt->key<keyMin) keyMin=nt->key;
    }

    selector->keyMax = keyMax;
    selector->keyMin = keyMin;
    selector->timestampEnd = timestampEnd;
}

static void _selectorVectorUpdateValuesOnNoteRemoval(struct notes_selection* selector, Note note) {
    if (!selector) return;
    if (!(selector->notesNum) || note->key<=selector->keyMin || note->key>=selector->keyMax || note->timestamp<=selector->timestampStart || note->timestamp+note->duration>=selector->timestampEnd) _selectorVectorUpdateValues(selector);
}

static void _selectorVectorAddLast(struct notes_selection* selector, Note note) {
    if (_initSelectorVector(selector) || !note) return;
    int idx = _selectorVectorFindWhereNoteShouldBe(selector, note);
    int existIdx = _selectorVectorFindNoteLinearlyAfter(selector, note, idx);
    if (existIdx>=0) return;
    if (selector->capacity<=selector->notesNum && _selectorVectorDuplicateCapacity(selector)) return;
    (selector->notes)[selector->notesNum]=note;
    _selectorVectorUpdateValuesOnNoteAddition(selector, note);
    (selector->notesNum)++;
}

static void _selectorVectorAdd(struct notes_selection* selector, Note note) {
    if (_initSelectorVector(selector) || !note) return;
    //(selector->notes)[(selector->notesNum)++] = note;
    //_selectorVectorSort(selector);
    int idx = _selectorVectorFindWhereNoteShouldBe(selector, note);
    int existIdx = _selectorVectorFindNoteLinearlyAfter(selector, note, idx);

    if (existIdx>=0) return;
    if (selector->capacity<=selector->notesNum && _selectorVectorDuplicateCapacity(selector)) return;
    memmove(selector->notes+idx+1, selector->notes+idx, (selector->notesNum-idx)*sizeof(Note));
    (selector->notes)[idx]=note;
    _selectorVectorUpdateValuesOnNoteAddition(selector, note);
    (selector->notesNum)++;
}

static void _selectorVectorRemove(struct notes_selection* selector, Note note) {
    if (_initSelectorVector(selector) || !note) return;
    int idx=_selectorVectorFind(selector, note), num=selector->notesNum;
    if (idx<0) return;
    (selector->notes)[idx]=NULL;
    if (num-idx-1) memmove(selector->notes+idx, selector->notes+idx+1, (num-idx-1)*sizeof(Note));
    if (((selector->capacity)>>2)>(--(selector->notesNum))) _selectorVectorHalveCapacity(selector);
    _selectorVectorUpdateValuesOnNoteRemoval(selector, note);
}

static void _selectorVectorToggle(struct notes_selection* selector, Note note) {
    if (_initSelectorVector(selector) || !note) return;
    int idx = _selectorVectorFindWhereNoteShouldBe(selector, note), num=selector->notesNum;;
    int existIdx = _selectorVectorFindNoteLinearlyAfter(selector, note, idx);
    if (existIdx>=0) {
        (selector->notes)[existIdx]=NULL;
        if (num-existIdx-1) memmove(selector->notes+existIdx, selector->notes+existIdx+1, (num-existIdx-1)*sizeof(Note));
        if (((selector->capacity)>>2)>(--(selector->notesNum))) _selectorVectorHalveCapacity(selector);
        _selectorVectorUpdateValuesOnNoteRemoval(selector, note);
    } else {
        if (selector->capacity<=(uint32_t)num && _selectorVectorDuplicateCapacity(selector)) return;
        memmove(selector->notes+idx+1, selector->notes+idx, (num-idx)*sizeof(Note));
        (selector->notes)[idx]=note;
        _selectorVectorUpdateValuesOnNoteAddition(selector, note);
        (selector->notesNum)++;
    }
}

static struct roll_rect _selectorGetRollRect(struct notes_selection* selector) {
    if (!selector || !(selector->clickHold)) return (struct roll_rect){.topLeft=(struct roll_point){.fkey=-5, .timestamp=0}, .bottomRight=(struct roll_point){.fkey=-5, .timestamp=0}};
    struct roll_rect rr = selector->rollRect;
    struct roll_point topLeft = {.fkey=floatMax(rr.topLeft.fkey, rr.bottomRight.fkey), .timestamp=uint32Min(rr.topLeft.timestamp, rr.bottomRight.timestamp)};
    struct roll_point bottomRight = {.fkey=floatMin(rr.topLeft.fkey, rr.bottomRight.fkey), .timestamp=uint32Max(rr.topLeft.timestamp, rr.bottomRight.timestamp)};
    return (struct roll_rect){.topLeft=topLeft, .bottomRight=bottomRight};
}

static struct roll_rect _selectorGetValueRect(struct notes_selection* selector) {
    if (!selector || !(selector->notesNum)) return (struct roll_rect){.topLeft=(struct roll_point){.fkey=-5, .timestamp=0}, .bottomRight=(struct roll_point){.fkey=-5, .timestamp=0}};
    return (struct roll_rect){.topLeft=(struct roll_point){.fkey=selector->keyMax+1, .timestamp=selector->timestampStart}, .bottomRight=(struct roll_point){.fkey=selector->keyMin, .timestamp=selector->timestampEnd}};
}

static void _selectorVectorSelectRegion(struct notes_selection* selector, uint8_t minKey, uint8_t maxKey, uint32_t minTimestamp, uint32_t maxTimestamp) {   // not included in primary selector
    if (!selector || selector->notes || minKey>127 || minKey>maxKey) return;

    //printf("_selectorVectorSelectRegion(): selector=%p, minKey=%u, maxKey=%u, minTimestamp=%u, maxTimestamp=%u\n", (void*)selector, minKey, maxKey, minTimestamp, maxTimestamp);

    Track track = globalStateHandler->project->tracks+globalStateHandler->selectedTrack;
    Note* trNotes = track->notes;
    uint32_t trackLen=track->numElements;
    uint32_t trackIdx=0;//_tracksGetTimestampIdx(track, minTimestamp);

    while (trackIdx<trackLen && (trNotes[trackIdx]->timestamp)<maxTimestamp) {
        Note note = trNotes[trackIdx++];
        if (note->key>maxKey || note->key<minKey || note->timestamp+note->duration<minTimestamp) continue;
        int existIdx = _selectorVectorFind(&(globalStateHandler->selector.primary), note);
        if (existIdx<0) _selectorVectorAddLast(selector, note);
    }
}

static void _selectorVectorUpdateGroupSelected(struct notes_selection* selector) {
    //printf("selector=%p, topLeft.fkey=%.2f, bottomRight.fkey=%.2f\n", (void*)selector, selector->rollRect.topLeft.fkey, selector->rollRect.bottomRight.fkey);
    if (!selector) return;
    if (selector->rollRect.topLeft.fkey<0 || selector->rollRect.bottomRight.fkey<0) {
        _selectorVectorReset(selector);
        return;
    }

    if (selector->notes) free(selector->notes);
    selector->notes = NULL;
    selector->capacity = 0;
    selector->notesNum = 0;

    uint8_t rangeKeyMin, rangeKeyMax;
    struct roll_rect rr = _selectorGetRollRect(selector);
    rangeKeyMin = (uint8_t)floor(rr.bottomRight.fkey);
    rangeKeyMax = (uint8_t)floor(rr.topLeft.fkey);

    _selectorVectorSelectRegion(selector, rangeKeyMin, rangeKeyMax, rr.topLeft.timestamp, rr.bottomRight.timestamp);
}

static void _mergeSortedVectors(Note* dest, const Note* a, uint32_t sa, const Note* b, uint32_t sb) {
    uint32_t id=0, sd=sa+sb, ia=0, ib=0;
    for (; id<sd; id++) {
        if (ia>=sa) dest[id]=b[ib++];
        else if (ib>=sb) dest[id]=a[ia++];
        else if (a[ia]->timestamp>b[ib]->timestamp) dest[id]=b[ib++];
        else dest[id]=a[ia++];
    }
}

static void _selectorsMergeAndClearNoChecks() {
    if (!globalStateHandler) return;
    struct notes_selection* primary = &(globalStateHandler->selector.primary);
    struct notes_selection* secondary = &(globalStateHandler->selector.secondary);

    if (!(secondary->notesNum)) {
        printf("Secondary doesn't have any notes\n");
        _selectorVectorReset(secondary);
        return;
    }

    uint32_t num = primary->notesNum + secondary->notesNum;
    if (!num) return;

    if (!(primary->notes)) {

        primary->capacity = secondary->capacity;
        primary->notesNum = secondary->notesNum;
        primary->notes = secondary->notes;

        primary->keyMax = secondary->keyMax;
        primary->keyMin = secondary->keyMin;
        primary->timestampStart = secondary->timestampStart;
        primary->timestampEnd = secondary->timestampEnd;

        // No sorting needed, since it's supposed to be already sorted in the secondary selector
        secondary->notes = NULL;
        secondary->notesNum = (secondary->capacity=0);
        _selectorVectorReset(secondary);
        
    } else {
        Note* arr = malloc(num*sizeof(Note));
        if (!arr) return;   // Failed

        _mergeSortedVectors(arr, primary->notes, primary->notesNum, secondary->notes, secondary->notesNum);     // In O(n) instead of O(nlog(n)) using qsort
        
        primary->capacity = num;
        primary->notesNum = num;
        free(primary->notes);
        primary->notes = arr;

        if (primary->keyMax<secondary->keyMax) primary->keyMax = secondary->keyMax;
        if (primary->keyMin>secondary->keyMin) primary->keyMin = secondary->keyMin;
        if (primary->timestampStart>secondary->timestampStart) primary->timestampStart = secondary->timestampStart;
        if (primary->timestampEnd<secondary->timestampEnd) primary->timestampEnd = secondary->timestampEnd;

        _selectorVectorReset(secondary);
    }
}


void globalHandlerAddNoteToSelected(Note note) {
    if (!globalStateHandler || !note) return;
    _selectorVectorAdd(&(globalStateHandler->selector.primary), note);
}

void globalHandlerRemoveSelectedNote(Note note) {
    if (!globalStateHandler || !note) return;
    _selectorVectorRemove(&(globalStateHandler->selector.primary), note);
}

void globalHandlerToggleSelectedNote(Note note) {
    if (!globalStateHandler || !note) return;
    _selectorVectorToggle(&(globalStateHandler->selector.primary), note);
}

int globalHandlerIsNoteSelected(Note note) {
    if (!globalStateHandler || !note) return 0;
    if (_selectorVectorFind(&(globalStateHandler->selector.primary), note)>=0) return 1;
    if (_selectorVectorFind(&(globalStateHandler->selector.secondary), note)>=0) return 1;
    return 0;
}

int globalHandlerGetNumberOfActuallySelectedNotes() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->selector.primary.notesNum;
}

int globalHandlerGetNumberOfVisuallySelectedNotes() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->selector.primary.notesNum+globalStateHandler->selector.secondary.notesNum;
}

void globalHandlerDeleteSelectedNotes() {
    if (!globalStateHandler || !(globalStateHandler->selector.primary.notesNum) || globalStateHandler->selectedTrack<0 || !(globalStateHandler->project->tracks)) return;
    uint32_t num = globalStateHandler->selector.primary.notesNum;
    int* indexes = malloc(num*sizeof(int));
    if (!indexes) return;

    Track track = globalStateHandler->project->tracks+globalStateHandler->selectedTrack;

    for (uint32_t i=0; i<num; i++) {
        indexes[i] = _tracksVectorFindWhereNoteShouldBe(track, (globalStateHandler->selector.primary.notes)[i]);
    }
    qsort(indexes, num, sizeof(int), _intCompareFunc);
    uint32_t idx=0;
    while (idx<num && indexes[idx]<0) idx++;
    for (uint32_t i=idx; i<num; i++) free((track->notes)[indexes[i]]);

    uint32_t read=0, write=0, delete=idx;
    while (read<track->numElements) {
        if (delete<num && indexes[delete]==(int)read) {
            delete++;
            read++;
            continue;
        }
        (track->notes)[write++] = (track->notes)[read++];
    }
    track->numElements = write;

    free(indexes);
    trackVectorResizeToFitJustNotes(track);
    globalHandlerClearNotesSelected();
}


void globalHandlerSelectGroupPress(float fkey, uint32_t timestamp) {
    if (!globalStateHandler || fkey>128 || fkey<0) return;
    globalStateHandler->selector.secondary.rollRect.topLeft = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    globalStateHandler->selector.secondary.rollRect.bottomRight = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    globalStateHandler->selector.secondary.clickHold = 1;
    _selectorVectorUpdateGroupSelected(&(globalStateHandler->selector.secondary));
}

void globalHandlerSelectGroupHold(float fkey, uint32_t timestamp) {
    if (!globalStateHandler || !(globalStateHandler->selector.secondary.clickHold)) return;
    if (fkey>=0 && fkey<128) globalStateHandler->selector.secondary.rollRect.bottomRight = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    globalStateHandler->selector.secondary.clickHold = 1;
    _selectorVectorUpdateGroupSelected(&(globalStateHandler->selector.secondary));
}

int globalHandlerIsSelectGroupActive() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->selector.secondary.clickHold;
}

struct roll_rect globalHandlerGetSelectGroupRect() {
    if (!globalStateHandler) return _selectorGetRollRect(NULL);
    return _selectorGetRollRect(&(globalStateHandler->selector.secondary));
}

void globalHandlerSelectGroupRelease(float fkey, uint32_t timestamp) {
    if (!globalStateHandler || !(globalStateHandler->selector.secondary.clickHold)) return;
    if (fkey>=0 && fkey<128) globalStateHandler->selector.secondary.rollRect.bottomRight = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    _selectorVectorUpdateGroupSelected(&(globalStateHandler->selector.secondary));
    globalStateHandler->selector.secondary.clickHold = 0;
    _selectorsMergeAndClearNoChecks();
}

void globalHandlerSelectGroupClear() {
    if (!globalStateHandler || !(globalStateHandler->selector.secondary.clickHold)) return;
    _selectorVectorReset(&(globalStateHandler->selector.secondary));
}


struct roll_rect globalHandlerGetCroppedRectangleForSelectedNotes() {
    if (!globalStateHandler) return _selectorGetValueRect(NULL);
    return _selectorGetValueRect(&(globalStateHandler->selector.primary));
}

uint32_t globalHandlerGetNumberOfSelectedNotes() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->selector.primary.notesNum;
}
// I should continue from the selectors (changes in both stateHndler.c and verticalKeyboard.c)