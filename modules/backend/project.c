#include <backend.h>
#include <stdlib.h>
#include <string.h>




struct project_saved {
    enum project_saved_state state;
    char* filepath;
    char* midipath;
};

typedef struct general_project_data {
    char* title;
    struct project_saved saveState;
    uint16_t tempo;
    struct time_signature timeSignature;
    uint16_t trackNum;
    Track tracks;
} *ProjectData;

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
    .trackNum = 0,
    .tracks = NULL
};
ProjectData globalProject = &_globalProject;




void freeProjectContents(ProjectData proj) {    // Doesn't free self
    if (!proj) return;
    if (proj->title) {
        free(proj->title);
        proj->title = NULL;
    }

    if (proj->saveState.filepath) {
        free(proj->saveState.filepath);
        proj->saveState.filepath=NULL;
    }

    if (proj->saveState.midipath) {
        free(proj->saveState.midipath);
        proj->saveState.midipath=NULL;
    }

    if (proj->trackNum && proj->tracks) {
        for (uint16_t i=0; i<proj->trackNum; i++) freeTrackContents(proj->tracks+i);
        free(proj->tracks);
        proj->tracks = NULL;
        proj->trackNum = 0;
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
    globalProject->trackNum = 0;
    globalProject->tracks = NULL;
    return 0;
}


char* projectGetCurrentTitle() {
    if (!globalProject) return NULL;
    return globalProject->title;
}