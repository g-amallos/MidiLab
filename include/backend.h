#ifndef BACKEND_H
#define BACKEND_H

#include <stdint.h>
#include <images.h>


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
void trackSetProgram(Track track, uint8_t program);
float trackGetPanning(Track track);
void trackSetPanning(Track track, float panning);
int trackGetSustain(Track track);
int trackGetProgram(Track track);



/* Handler (backend/stateHandler.c) */

enum keyboard_render_types {
    T_KEYBOARD_HORIZONTAL,
    T_KEYBOARD_VERTICAL,
    T_KEYBOARD_NONE
};


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
enum keyboard_render_types globalStateHandlerGetKeyboardType();
void globalHandlerUpdateKey(int key, uint8_t velocity);
void globalHandlerUpdateKeyAndPlaySynth(int key, uint8_t velocity);



/* Actions (backend/actions.c) */

void actionExecuteAllDeferred();
void actionExecuteAndRemoveFirst();
int actionIsQueueEmpty();
void actionDefer(OnClickFunc func);





/* Midi (backend/midi.c) */

typedef struct midi_program *MidiProgram;

enum midi_program_type {
    MPT_PIANO,
    MPT_CHROMATIC_PERCUSSION,
    MPT_ORGAN,
    MPT_GUITAR,
    MPT_BASS,
    MPT_STRINGS,
    MPT_ENSEMBLE,
    MPT_BRASS,
    MPT_REED,
    MPT_PIPE,
    MPT_SYNTH_LEAD,
    MPT_SYNTH_PAD,
    MPT_SYNTH_EFFECTS,
    MPT_ETHNIC,
    MPT_PERCUSSIVE,
    MPT_SOUND_EFFECTS,
    MPT_DRUMS,
    MPT_END
};


struct midi_programs_array {
    int num;
    MidiProgram* array;
};


void midiInit();
void midiClose();
const char* midiGetProgramTypeString(int program);
enum icon_title midiGetProgramTypeIcon(int program);

const char* midiGetProgramName(int program);
const char* midiProgramGetName(MidiProgram program);
const char* midiProgramGetTypeString(MidiProgram program);
enum icon_title midiGetTypeIconFromType(enum midi_program_type type);
enum icon_title midiProgramGetTypeIcon(MidiProgram program);
enum midi_program_type midiGetProgramType(int program);
enum midi_program_type midiProgramGetType(MidiProgram program);

int midiProgramGetProgramNum(MidiProgram program);
struct midi_programs_array midiGetProgramsByType(enum midi_program_type type);
void midiFreeMidiProgramArray(struct midi_programs_array* mpa);


#endif