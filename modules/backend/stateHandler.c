#include "backend_internal.h"
#include <stdlib.h>
#include <string.h>
#include <synth.h>
#include <utils.h>
#include <stdio.h>



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
    .selectedTrack = -1
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