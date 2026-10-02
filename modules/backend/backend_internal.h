#ifndef BACKEND_INTERNAL_H
#define BACKEND_INTERNAL_H

#include <backend.h>




enum project_saved_state {
    S_STATE_UNSAVED_PROJECT,
    S_STATE_SAVED,
    S_STATE_UNSAVED_CHANGES,
    
    S_STATE_END
};

// Each beat should be sliced in 2^14 pieces, leaving a decent space of 2^22 beats per project




typedef struct track_data {
    char* title;
    uint8_t program;
    uint8_t channel;
    uint8_t sustain;
    
    float velocity;
    float panning;

    uint8_t keyMin;
    uint8_t keyMax;
    uint32_t timestampStart;
    uint32_t timestampEnd;

    uint32_t capacity;      // Allocating more than needed, for fewer realloc calls
    uint32_t numElements;      // Number of actual saved note data (the first n in the array)
    Note* notes;

} *Track;



struct time_line {
    uint8_t playing;
    uint8_t loopEnabled;
    uint8_t timeShown;

    uint32_t timestamp;
    uint32_t prePlayTimestamp;
    double time;    // The vertical line that shows the time, obtained with `globalHandlerGetLineTime()`
    double prePlayTime;

    double loopStart;
    double loopEnd;
};


struct backend_time_handler {
    double time;    // Start of the div has this timestamp, obtained with `globalHandlerGetTime()`
    uint64_t ticks;
    uint32_t mouseJumps;    // Visual mouse jumps depending on the div zoom

    uint16_t tempo;
    uint16_t ppqn;  // Pulses per quarter note
    uint16_t ticksPerBeat;

    struct time_signature timeSignature;
    double measureDuration;
    double beatDuration;
    double visibleDuration;

    struct time_line timeline;
};



struct key_state {
    uint8_t velocity;
    uint8_t changed;
};

struct backend_keys_handler {
    enum keyboard_render_types type;

    uint8_t inputAllowed;
    struct key_state keys[128];
};

struct reference_note {
    Note note;
    uint32_t originalTimestamp;
    uint32_t minPossibleTimestamp;
    uint8_t originalKey;
    uint8_t minPossibleKey;
    uint8_t maxPossibleKey;    
};

struct notes_selection {
    uint8_t keyMin;
    uint8_t keyMax;

    uint8_t clickHold;

    struct roll_rect rollRect;
    struct reference_note noteReference;

    uint32_t timestampStart;
    uint32_t timestampEnd;

    uint32_t capacity;
    uint32_t notesNum;
    Note* notes;
};

struct notes_selector {
    struct notes_selection primary;
    struct notes_selection secondary;
    struct notes_selection clipboard;
};

typedef struct backend_state_handler {
    struct backend_time_handler time;
    struct backend_keys_handler keys;
    ProjectData project;

    int selectedTrack;

    struct notes_selector selector;
    enum app_render_type renderType;
    enum visualizer_type visualizerType;

} *StateHandler;

extern StateHandler globalStateHandler;

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
    uint16_t tracksNum;
    Track tracks;
} *ProjectData;

extern struct general_project_data _globalProject;


enum midi_message_types {
    MM_NOTE_ON,
    MM_NOTE_OFF,
    MM_PROGRAM_CHANGE,
};

struct midi_event_note_onoff {
    uint8_t key;
    uint8_t velocity;
};

struct midi_event { // NOT FINISHED!!!
    int channel;
    int track;
    enum midi_message_types type;

    union {
        struct {
            uint8_t key;
            uint8_t velocity;
            uint8_t panning;
        } note_on;

        struct {
            uint8_t key;
            uint8_t velocity;
        } note_off;

        struct {
            uint8_t program;
        } program_change;

    };
};



typedef struct midi_program {
    char* name;
    enum midi_program_type type;
    int icon;

} *MidiProgram;

typedef struct midi_drum {
    char* name;
} *MidiDrum;

#define MIDI_DRUM_START 35
#define MIDI_DRUM_END 81

extern struct midi_program _midiPrograms[129];
extern struct midi_drum _midiDrums[128];


void globalStateHandlerInit();
void _globalHandlerUpdateDurations();
void _globalStateHandlerUpdateTempo(double tempo);

double timestampPiecesToSamples(uint32_t pieces, uint32_t sampleRate);
double samplesToTimestampPieces(uint32_t samples, uint32_t sampleRate);

void projectSetSaveStatus(enum project_saved_state status);
const char* projectGetSavedFilepath();
void updateWindowProjectTitle();
void trackLoadTmpTrack(Track dest, Track src);

void trackHalveCapacity(Track track);
void trackVectorResizeToFitJustNotes(Track track);

void trackRecalculateValues(Track track);
void trackSortNotes(Track track);
void sortNoteBuffer(Note* buffer, uint64_t size);
void trackVectorAddNewNotes(Track track, Note* buff, uint32_t size);    // buff must have sorted notes
void _mergeSortedVectors(Note* dest, const Note* a, uint32_t sa, const Note* b, uint32_t sb);

void _exportSetupSynthTracks();


#endif