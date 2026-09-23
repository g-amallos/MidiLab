#ifndef BACKEND_H
#define BACKEND_H

#include <stdint.h>
#include <images.h>


typedef void (*OnClickFunc)(void);
typedef struct midi_event* MidiEvent;


typedef struct general_project_data *ProjectData;
typedef struct backend_state_handler *StateHandler;
typedef struct track_data *Track;
typedef struct note_data *Note;

struct time_signature {
    uint8_t numerator;
    uint8_t denominator;
};


struct tiles_info {
    uint8_t minKey;
    uint8_t maxKey;
    uint8_t minShownKey;
    uint8_t maxShownKey;
    uint16_t keys;
    uint16_t shownKeys; 
    
    uint32_t tracks;
    uint32_t notes;
    double duration;
};


/* General (backend/general.c) */

int backendInit();      // Initialize the backend
int backendClose();     // Close and free the backend


/* Project (backend/project.c) */

struct duration_data {
    uint32_t timestamp;
    double time;
};

struct note_array {
    uint64_t size;
    Note* notes;
};

extern ProjectData globalProject;               // The reference the whole program will use for the project

int createNewProject();                         // Updates the global loaded project to a new one
void freeProjectContents();                     // Frees whatever can be freed from the ProjectData (Doesn't free self)
void newProject();                              // Creates a new project (both UI and backend)

const char* projectGetCurrentTitle();           // Hiding the implementation
void projectSetCurrentTitle(const char* text);  // Set/Update the title. Copies the text
int projectSetTempo(int tempo);                 // Updates the tempo and returns the tempo that has been set
int projectGetTempo();
int projectGetTracksNum();
uint32_t projectGetTotalNumberOfNotes();
uint16_t projectGetMaxTitleLength();
void projectSetFilepath(const char* filepath);
void projectUpdateStateSomethingChanged();
int projectHasUnsavedChanges();
int projectHasSavedFilepath();
int projectIsPracticallyEmpty();
int projectCanSafelyReplaceContents();
int projectClose();
struct duration_data projectGetDuration();
struct tiles_info projectGetTilesInfo(int showDrums);
struct note_array projectGetNoteArray(int showDrums);
void projectFreeNoteArray(struct note_array noteArray);


/* Track (backend/tracks.c) */


typedef struct note_data {
    uint8_t key;
    uint8_t velocity;
    uint8_t channel;
    uint8_t track;
    uint32_t timestamp;     // Relative
    uint32_t duration;      // Relative
    double ftimestamp;
    double fduration;
} *Note;



void freeTrackContents(Track track);            // Frees whatever can be freed from the Track (Doesn't free self)
Track trackCreateNew();
Track trackGetAtIdx(int idx);
Track trackGetSelectedTrack();
const char* trackGetTitle(Track track);
void trackSetTitle(Track track, const char* title);
float trackGetVelocity(Track track);
void trackSetVelocity(Track track, float velocity);
int trackGetProgram(Track track);
void trackDeleteAtIdx(int idx);
int trackMoveToIndex(Track track, int idx);
int trackCanSafelyReplaceContents(Track track);
void trackSetProgram(Track track, uint8_t program);
float trackGetPanning(Track track);
void trackSetPanning(Track track, float panning);
int trackGetSustain(Track track);
int trackGetProgram(Track track);
Note trackCreateNoteInTrack(Track track, uint8_t note, uint8_t velocity, uint32_t timestamp, uint32_t duration);
void trackDeleteNoteInTrackByIdx(Track track, uint32_t idx);
void trackDeleteNoteInTrack(Track track, Note note);
uint32_t trackPiecesInBeat();
uint32_t trackGetNumOfNotes(Track track);
Note* trackGetNotes(Track track);
uint16_t tracksGetMaxTracks();
uint16_t trackGetMaxTitleLength();
int tracksCanCreateNew();
int trackGetMinKey(Track track);
int trackGetMaxKey(Track track);
uint32_t trackGetTimestampEnd(Track track);
void tracksUpdateAllValues();
void trackUpdateAllNoteData(Track track);


/* Handler (backend/stateHandler.c) */

enum app_render_type {
    ART_REGULAR,
    ART_VISUALIZER,
    ART_VERTICAL_TILES,
    ART_NONE
};

enum visualizer_type {
    VISUALIZER_TYPE_1,
    VISUALIZER_TYPE_2,
    VISUALIZER_TYPE_3,
    VISUALIZER_TYPE_4
};

enum keyboard_render_types {
    T_KEYBOARD_HORIZONTAL,
    T_KEYBOARD_VERTICAL,
    T_KEYBOARD_NONE
};

struct roll_point {
    float fkey;
    uint32_t timestamp;
};

struct roll_rect {
    struct roll_point topLeft;
    struct roll_point bottomRight;
};


extern StateHandler globalStateHandler;         // Another reference the whole program will use for the project
double globalHandlerGetTime();
void globalHandlerSetTime(double time);
int globalHandlerIsPlaying();
void globalHandlerPlay();
void globalHandlerPause();
void globalHandlerEnableLoop();
void globalHandlerDisableLoop();
int globalHandlerIsLoopEnabled();
void globalHandlerSelectTrack(int idx);
struct time_signature globalHandlerGetTimeSignature();
void globalHandlerSetTimeSignature(struct time_signature tsign);
int globalHandlerGetSelectedTrack();
enum keyboard_render_types globalStateHandlerGetKeyboardType();
void globalHandlerSetKeyboardType(enum keyboard_render_types view);
void globalHandlerUpdateKey(int key, uint8_t velocity);
void globalHandlerUpdateKeyAndPlaySynth(int key, uint8_t velocity);
double globalHandlerGetVisibleDuration();
double globalHandlerDurationToMeasures(double seconds);
double globalHandlerMeasuresToDuration(double measures);
int globalHandlerGetBeatsInMeasure();
double globalHandlerGetMeasureDuration();
double globalHandlerGetBeatDuration();
double globalHandlerTimestampToSeconds(uint32_t timestamp);
void globalHandlerSetVisibleDuration(double duration);
void globalHandlerSetVisibleMouseJumps(uint32_t mouseJumps);
int globalHandlerIsTimeLineShown();
double globalHandlerGetLineTime();
void globalHandlerSetLineTime(double time);
uint32_t globalHandlerGetLineTimestamp();
uint32_t globalHandlerGetTimestamp();
void globalHandlerUpdateTick();
uint32_t secondsToTimestamp(double seconds);
enum app_render_type globalHandlerGetRenderType();
void globalHandlerSetRenderType(enum app_render_type type);
enum visualizer_type globalHandlerGetVisualizerType();
void globalHandlerSetVisualizerType(enum visualizer_type visType);
void globalHandlerToggleNextVisualization();


void globalHandlerUpdateSelectedTrack();
void globalHandlerUpdateAllTracks();

void globalHandlerSetToNextMeasure();
void globalHandlerSetToPreviousMeasure();

void globalHandlerClearNotesSelected();
void globalHandlerAddNoteToSelected(Note note);
void globalHandlerRemoveSelectedNote(Note note);
void globalHandlerToggleSelectedNote(Note note);
void globalHandlerSelectSingleNote(Note note);
int globalHandlerIsNoteSelected(Note note);
int globalHandlerGetNumberOfActuallySelectedNotes();
int globalHandlerGetNumberOfVisuallySelectedNotes();
void globalHandlerDeleteSelectedNotes();

void globalHandlerSelectGroupPress(float fkey, uint32_t timestamp);
void globalHandlerSelectGroupHold(float fkey, uint32_t timestamp);
void globalHandlerSelectGroupRelease(float fkey, uint32_t timestamp);
int globalHandlerIsSelectGroupActive();
uint32_t globalHandlerGetNumberOfSelectedNotes();
void globalHandlerSelectGroupClear();
struct roll_rect globalHandlerGetSelectGroupRect();
struct roll_rect globalHandlerGetCroppedRectangleForSelectedNotes();
void globalHandlerChangeVelocityOfSelectedNotes(uint8_t velocity);
void globalHandlerMoveSelectedPress(Note note, float fkey, uint32_t timestamp);
void globalHandlerMoveSelectedHold(float fkey, uint32_t timestamp);
void globalHandlerMoveSelectedRelease(float fkey, uint32_t timestamp);
int globalHandlerMoveSelectedIsActive();
void globalHandlerResizeSelectedPress(Note note, float fkey, uint32_t timestamp);
void globalHandlerResizeSelectedHold(float fkey, uint32_t timestamp);
uint32_t globalHandlerResizeSelectedRelease(float fkey, uint32_t timestamp);
Note globalHandlerResizeSelectedGetReferenceNote();
int globalHandlerResizeSelectedIsActive();
void globalHandlerSelectAllNotes();

void globalHandlerClearClipboard();
void globalHandlerCopySelected();
void globalHandlerCutSelected();
void globalHandlerPasteSelected();


/* Actions (backend/actions.c) */

void actionExecuteAllDeferred();
void actionExecuteAndRemoveFirst();
int actionIsQueueEmpty();
void actionDefer(OnClickFunc func);
void actionClose();

void midiActionAdd(MidiEvent event, double time);
void midiActionExecuteFrame();
void midiActionRemoveAll();         // Doesn't execute anything, only deletes all registered events
void midiActionClose();





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


typedef struct midi_event* MidiEvent;


void midiInit();
void midiClose();
const char* midiGetProgramTypeString(int program);
enum icon_title midiGetProgramTypeIcon(int program);

const char* midiGetDrumName(int note);
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


void midiEventFree(MidiEvent event);
MidiEvent midiCreateEventForNoteOn(Note note, float volume);
MidiEvent midiCreateEventForNoteOff(Note note);


#endif