#ifndef BACKEND_INTERNAL_H
#define BACKEND_INTERNAL_H

#include <backend.h>




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
    uint8_t sustain;
    
    float velocity;
    float panning;

    uint32_t internalElements;      // Allocating more than needed, for fewer realloc calls
    uint32_t externalElements;      // Number of actual saved note data (the first n in the array)
    Note notes;
} *Track;


struct time_signature {
    uint8_t numerator;
    uint8_t denominator;
};



struct backend_time_handler {
    double time;

    uint16_t tempo;
    struct time_signature timeSignature;
    double measureDuration;

    uint8_t playing ;
    uint8_t loopEnabled;
    double loopStart;
    double loopEnd;
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

typedef struct backend_state_handler {
    struct backend_time_handler time;
    struct backend_keys_handler keys;
    ProjectData project;
    int selectedTrack;


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






typedef struct midi_program {
    char* name;
    enum midi_program_type type;
    int icon;

} *MidiProgram;


extern struct midi_program _midiPrograms[129];



#endif