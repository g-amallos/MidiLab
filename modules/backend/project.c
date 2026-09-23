#include "backend_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <handler.h>
#include <utils.h>
#include <string.h>


void updateCLTextboxes();                               // modules/interface/render/controlLine.c
void freeTrackUIs();                                    // modules/interface/render/tracks.c


static char* _title = NULL;



struct general_project_data _globalProject = {
    .title = NULL,
    .saveState = {
        .state = S_STATE_UNSAVED_PROJECT,
        .filepath = NULL,
        .midipath = NULL
    },
    .tempo = 120,
    .timeSignature = {
        .numerator = 4,
        .denominator = 4
    },
    .tracksNum = 0,
    .tracks = NULL
};
ProjectData globalProject = &_globalProject;




void freeProjectContents() {
    if (!globalProject) return;
    if (globalProject->title) {
        free(globalProject->title);
        globalProject->title = NULL;
    }

    if (globalProject->saveState.filepath) {
        free(globalProject->saveState.filepath);
        globalProject->saveState.filepath=NULL;
    }

    if (globalProject->saveState.midipath) {
        free(globalProject->saveState.midipath);
        globalProject->saveState.midipath=NULL;
    }

    if (globalProject->tracksNum && globalProject->tracks) {
        for (uint16_t i=0; i<globalProject->tracksNum; i++) freeTrackContents(globalProject->tracks+i);
        free(globalProject->tracks);
        globalProject->tracks = NULL;
        globalProject->tracksNum = 0;
    }
}


int createNewProject() {
    char s[] = "New Project";
    char* newTitle = strdup(s);
    if (!newTitle) return 1;

    freeProjectContents();
    globalProject->title = newTitle;
    globalProject->saveState.state = S_STATE_UNSAVED_PROJECT;
    globalProject->saveState.filepath = NULL;
    globalProject->saveState.midipath = NULL;

    globalProject->tempo = 120;
    globalProject->timeSignature.numerator = 4;
    globalProject->timeSignature.denominator = 4;
    globalProject->tracksNum = 0;
    globalProject->tracks = NULL;

    
    return 0;
}

void newProject() {
    if (createNewProject()) return;
    
    uint16_t tempo = globalProject->tempo;
    struct time_signature ts = globalProject->timeSignature;
    
    globalHandlerSetTimeSignature(ts);
    projectSetTempo(tempo);
    globalHandlerSetKeyboardType(T_KEYBOARD_NONE);

    freeTrackUIs();
    updateCLTextboxes();
}


const char* projectGetCurrentTitle() {
    if (!globalProject) return NULL;
    return globalProject->title;
}

void projectSetCurrentTitle(const char* text) {
    if (!globalProject) return;
    int len = strlen(text);
    char* new = malloc((len+1)*sizeof(char));
    if (!new) return;

    if (globalProject->title) free(globalProject->title);

    globalProject->title = new;
    strncpy(new, text, len+1);
    projectUpdateStateSomethingChanged();
}

int projectSetTempo(int tempo) {
    if (!globalProject) return 0;
    if (tempo>2400) tempo=2400;
    if (tempo<30) tempo=30;
    globalProject->tempo = tempo;
    _globalStateHandlerUpdateTempo(tempo);
    _globalHandlerUpdateDurations();
    projectUpdateStateSomethingChanged();
    return tempo;
}

int projectGetTempo() {
    if (!globalProject) return 0;
    return globalProject->tempo;
}

int projectGetTracksNum() {
    if (!globalProject) return 0;
    return globalProject->tracksNum;
}

void projectLoadTmpProject(ProjectData newProject) {
    globalHandlerPause();

    uint16_t tempo = newProject->tempo;
    struct time_signature ts = newProject->timeSignature;

    freeProjectContents();
    *globalProject = *newProject;

    tracksUpdateAllValues();

    globalHandlerSetTimeSignature(ts);
    projectSetTempo(tempo);

    updateCLTextboxes();
    globalHandlerUpdateAllTracks();
}

void projectSetFilepath(const char* filepath) {
    if (!globalProject) return;

    if (globalProject->saveState.filepath) free(globalProject->saveState.filepath);
    globalProject->saveState.filepath = NULL;
    if (filepath) {
        char* path = strdup(filepath);
        globalProject->saveState.filepath = path;
    }
}

void projectSetSaveStatus(enum project_saved_state status) {
    if (!globalProject || status>=S_STATE_END || status<S_STATE_UNSAVED_PROJECT) return;
    globalProject->saveState.state = status;
    updateWindowProjectTitle();
}

void projectUpdateStateSomethingChanged() {
    if (!globalProject || globalProject->saveState.state==S_STATE_UNSAVED_PROJECT) {
        updateWindowProjectTitle();
        return;
    }
    if (globalProject->saveState.state==S_STATE_SAVED) globalProject->saveState.state=S_STATE_UNSAVED_CHANGES;
    updateWindowProjectTitle();
}

int projectHasUnsavedChanges() {
    if (!globalProject || globalProject->saveState.state!=S_STATE_SAVED) return 1;
    return 0;
}

int projectHasSavedFilepath() {
    return (globalProject && globalProject->saveState.filepath);
}

const char* projectGetSavedFilepath() {
    if (!globalProject) return NULL;
    return globalProject->saveState.filepath;
}

int projectIsPracticallyEmpty() {
    if (!globalProject) return 0;
    int totalNotes=0, totalTracks=(int)(globalProject->tracksNum);

    for (int i=0; i<totalTracks; i++) {
        totalNotes += (globalProject->tracks)[i].numElements;
        if (totalNotes>0) return 0;
    }
    return 1;
}

int projectCanSafelyReplaceContents() {
    if (!globalProject) return 0;
    if (globalProject->saveState.state==S_STATE_SAVED && globalProject->saveState.filepath) return 1;   // Project saved
    return projectIsPracticallyEmpty();                                                                 // Project is empty
}

void updateWindowProjectTitle() {
    static int saved = 0;

    int newSaved = (globalProject->saveState.state==S_STATE_SAVED);
    char* newTitle = globalProject->title;

    if (saved!=newSaved || stringCompareWrapper(_title, newTitle)) {
        saved = newSaved;
        if (_title) free(_title);
        _title = NULL;

        if (newTitle) _title = strdup(newTitle);
        updateWindowTitle(_title, saved);
    }
}

uint16_t projectGetMaxTitleLength() {
    return 40;
}


int projectClose() {
    if (_title) free(_title);

    return 0;
}


uint32_t projectGetTotalNumberOfNotes() {
    if (!globalProject) return 0;
    uint32_t ret=0;
    uint16_t i=0, n=globalProject->tracksNum;
    for (; i<n; i++) ret+=(globalProject->tracks)[i].numElements;
    return ret;
}


struct duration_data projectGetDuration() {
    if (!globalProject) return (struct duration_data){0,0};
    uint32_t maxDur=0;
    uint16_t n=globalProject->tracksNum;
    for (uint16_t i=0; i<n; i++) {
        Track track = globalProject->tracks+i;
        trackRecalculateValues(track);
        uint32_t td = trackGetTimestampEnd(track);
        if (td>maxDur) maxDur=td;
    }
    return (struct duration_data){.timestamp=maxDur, .time=globalHandlerTimestampToSeconds(maxDur)};
}

struct tiles_info projectGetTilesInfo(int showDrums) {
    struct tiles_info info = {.minKey=127, .maxKey=0, .minShownKey=127, .maxShownKey=0, .keys=0, .shownKeys=0, .tracks=0, .notes=0, .duration=0};
    int minKey=127, maxKey=0;

    if (globalProject && (globalProject->tracksNum) && (globalProject->tracks)) {
        uint32_t maxDur=0;
        uint16_t n=globalProject->tracksNum;
        for (uint16_t i=0; i<n; i++) {
            Track track = globalProject->tracks+i;
            uint32_t notes = trackGetNumOfNotes(track);
            int program = trackGetProgram(track);

            if (!notes || (!showDrums && program>127)) continue;

            info.tracks++;
            info.notes += notes;
            trackRecalculateValues(track);
            uint32_t td = trackGetTimestampEnd(track);
            if (td>maxDur) maxDur=td;
            uint8_t mink = trackGetMinKey(track);
            if (mink<minKey) minKey=mink;
            uint8_t maxk = trackGetMaxKey(track);
            if (maxk>maxKey) maxKey=maxk;
        }
        info.duration = globalHandlerTimestampToSeconds(maxDur);
    }


    info.minKey = minKey;
    info.maxKey = maxKey;
    info.keys = 1+maxKey-minKey;

    if (minKey>maxKey) {
        minKey=60, maxKey=64;
    }
    while (maxKey-minKey<18) {
        if (minKey>0) minKey--;
        if (maxKey<127) maxKey++;
    }

    while (1) {
        uint8_t t=minKey%12;
        if (minKey<0) minKey=0;
        if (t==5 || t==0) break;
        minKey--;
    }
    while (1) {
        uint8_t t=maxKey%12;
        if (maxKey>127) minKey=131;
        if (t==11 || t==4) break;
        maxKey++;
    }


    info.minShownKey = minKey;
    info.maxShownKey = maxKey;
    info.shownKeys = 1+maxKey-minKey;

    
    return info;
}


struct note_array projectGetNoteArray(int showDrums) {
    struct note_array ret={0,NULL};
    if (!globalProject || !(globalProject->tracksNum) || !(globalProject->tracks)) return ret;

    uint64_t size=0;
    uint16_t n=globalProject->tracksNum;
    for (uint16_t i=0; i<n; i++) {
        Track track = globalProject->tracks+i;
        uint32_t notes = trackGetNumOfNotes(track);
        int program = trackGetProgram(track);

        if (!notes || (!showDrums && program>127)) continue;

        size += notes;
        trackRecalculateValues(track);
    }
    if (!size) return ret;

    Note* buffer = malloc(sizeof(Note)*size);
    if (!buffer) return ret;
    ret.notes = buffer;
    ret.size = size;

    uint64_t startingIdx=0;
    for (uint16_t i=0; i<n; i++) {
        Track track = globalProject->tracks+i;
        uint32_t notes = trackGetNumOfNotes(track);
        int program = trackGetProgram(track);

        if (!notes || (!showDrums && program>127)) continue;

        trackUpdateAllNoteData(track);
        Note* arr = trackGetNotes(track);

        memmove(buffer+startingIdx, arr, notes*sizeof(Note));
        startingIdx += notes;
    }

    sortNoteBuffer(buffer, size);
    return ret;
}

void projectFreeNoteArray(struct note_array noteArray) {
    if (noteArray.notes) free(noteArray.notes);
}