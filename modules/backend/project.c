#include "backend_internal.h"
#include <stdlib.h>
#include <string.h>






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
}

int projectSetTempo(int tempo) {
    if (!globalProject) return 0;
    if (tempo>2400) tempo=2400;
    if (tempo<30) tempo=30;
    globalProject->tempo = tempo;
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