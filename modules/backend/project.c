#include "backend_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <handler.h>
#include <utils.h>


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

    freeProjectContents(globalProject);
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