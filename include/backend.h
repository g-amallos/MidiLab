#ifndef BACKEND_H
#define BACKEND_H

#include <stdint.h>


typedef void (*OnClickFunc)(void);



typedef struct general_project_data *ProjectData;
typedef struct backend_state_handler *StateHandler;
typedef struct track_data *Track;
typedef struct note_data *Note;





/* General (backend/general.c) */

int backendInit();      // Initialize the backend
int backendClose();     // Close and free the backend


/* Project (backend/project.c) */

extern ProjectData globalProject;               // The reference the whole program will use for the project

int createNewProject();                         // Updates the global loaded project to a new one
void freeProjectContents();                     // Frees whatever can be freed from the ProjectData (Doesn't free self)

const char* projectGetCurrentTitle();           // Hiding the implementation
void projectSetCurrentTitle(const char* text);  // Set/Update the title. Copies the text
int projectSetTempo(int tempo);                 // Updates the tempo and returns the tempo that has been set
int projectGetTempo();
int projectGetTracksNum();

/* Track (backend/tracks.c) */

void freeTrackContents(Track track);            // Frees whatever can be freed from the Track (Doesn't free self)
Track trackCreateNew();
Track trackGetAtIdx(int idx);
const char* trackGetTitle(Track track);
void trackSetTitle(Track track, const char* title);
float trackGetVelocity(Track track);
void trackSetVelocity(Track track, float velocity);
int trackGetProgram(Track track);
void trackDeleteAtIdx(int idx);




/* Handler (backend/stateHandler.c) */

extern StateHandler globalStateHandler;         // Another reference the whole program will use for the project
double globalHandlerGetTime();
int globalHandlerIsPlaying();
void globalHandlerPlay();
void globalHandlerPause();
void globalHandlerEnableLoop();
void globalHandlerDisableLoop();
int globalHandlerIsLoopEnabled();
void globalHandlerSelectTrack(int idx);
int globalHandlerGetSelectedTrack();



/* Actions (backend/actions.c) */

void actionExecuteAllDeferred();
void actionExecuteAndRemoveFirst();
int actionIsQueueEmpty();
void actionDefer(OnClickFunc func);

#endif