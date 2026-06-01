#include "backend_internal.h"
#include <stdlib.h>
#include <string.h>
#include <synth.h>
#include <utils.h>




struct backend_state_handler _globalHandler = {
    .time = {
        .time = 0,
        .ticks = 0,

        
        .tempo = 120,
        .ppqn = 480,
        .ticksPerBeat = 440,
        .measureDuration = 0,
        .beatDuration = 0,
        .visibleDuration = 30,

        .timeSignature = {
            .numerator = 4,
            .denominator = 4
        },

        .timeline = {
            .playing = 0,
            .timeShown = 1,
            .loopEnabled = 0,

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
}

void globalHandlerPause() {
    if (!globalStateHandler) return;
    globalStateHandler->time.timeline.playing = 0;
    globalStateHandler->time.timeline.time = globalStateHandler->time.timeline.prePlayTime;
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




void globalHandlerUpdateTick() {

    float dt = GetFrameTime();

    if (globalStateHandler->time.timeline.playing) {
        struct backend_time_handler th = globalStateHandler->time;

        double oldT = th.timeline.time;
        double newT = (globalStateHandler->time.timeline.time+=dt);


        // search all tracks for notes between these

        globalStateHandler->time.time = doubleMax(0*oldT*newT, th.timeline.time-0.35*th.visibleDuration);


    } else {

    }
}