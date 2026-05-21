#include "backend_internal.h"
#include <stdlib.h>
#include <string.h>
#include <synth.h>




struct backend_state_handler _globalHandler = {
    .time = {
        .time = 0,
        .playing = 0,
        .tempo = 120,
        .measureDuration = 0,
        .visibleDuration = 30,

        .timeSignature.numerator = 4,
        .timeSignature.denominator = 4,
        
        .timeShown = 0,
        .loopEnabled = 0,
        .loopStart = 0,
        .loopEnd = 0
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



double globalHandlerGetTime() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.time;
}

int globalHandlerIsPlaying() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.playing;
}

void globalHandlerPlay() {
    if (!globalStateHandler) return;
    globalStateHandler->time.playing = 1;
}

void globalHandlerPause() {
    if (!globalStateHandler) return;
    globalStateHandler->time.playing = 0;
}

void globalHandlerEnableLoop() {
    if (!globalStateHandler) return;
    globalStateHandler->time.loopEnabled = 1;
}

void globalHandlerDisableLoop() {
    if (!globalStateHandler) return;
    globalStateHandler->time.loopEnabled = 0;
}

int globalHandlerIsLoopEnabled() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->time.loopEnabled;
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