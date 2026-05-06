#ifndef BACKEND_H
#define BACKEND_H

#include <stdint.h>


enum project_saved_state {
    S_STATE_UNSAVED_PROJECT,
    S_STATE_SAVED,
    S_STATE_UNSAVED_CHANGES,
    
    S_STATE_END
};

typedef struct note_data {
    uint8_t key;
    uint8_t velocity;
    uint8_t channel;
    uint8_t track;          // Not needed field, so we can change that in the future
    uint32_t timestamp;     // Relative
    uint32_t duration;      // Relative
} *Note;

typedef struct track_data {
    char* title;
    uint8_t program;
    uint8_t channel;
    uint8_t idk;
    float velocity;

    uint32_t internalElements;      // Allocating more than needed, for fewer realloc calls
    uint32_t externalElements;      // Number of actual saved note data (the first n in the array)
    Note notes;
} *Track;


struct time_signature {
    uint8_t numerator;
    uint8_t denominator;
};



typedef struct general_project_data *ProjectData;






/* General (backend/general.c) */

int backendInit();      // Initialize the backend
int backendClose();     // Close and free the backend


/* Project (backend/project.c) */

extern ProjectData globalProject;               // The reference the whole program will use for the project

int createNewProject();                         // Updates the global loaded project to a new one
void freeProjectContents(ProjectData proj);     // Frees whatever can be freed from the ProjectData (Doesn't free self)

const char* projectGetCurrentTitle();           // Hiding the implementation
void projectSetCurrentTitle(const char* text);  // Set/Update the title. Copies the text
int projectSetTempo(int tempo);                 // Updates the tempo and returns the tempo that has been set
int projectGetTempo();

/* Track (backend/tracks.c) */

void freeTrackContents(Track track);            // Frees whatever can be freed from the Track (Doesn't free self)



#endif