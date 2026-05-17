#include "backend_internal.h"
#include <images.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


struct midi_program _midiPrograms[129] = {{NULL,0,0}};

const char* programTypeStrings[MPT_END] = {
    [MPT_PIANO] = "Piano",
    [MPT_CHROMATIC_PERCUSSION] = "Chromatic Percussion",
    [MPT_ORGAN] = "Organ",
    [MPT_GUITAR] = "Guitar",
    [MPT_BASS] = "Bass",
    [MPT_STRINGS] = "Strings",
    [MPT_ENSEMBLE] = "Ensemble",
    [MPT_BRASS] = "Brass",
    [MPT_REED] = "Reed",
    [MPT_PIPE] = "Pipe",
    [MPT_SYNTH_LEAD] = "Synth Lead",
    [MPT_SYNTH_PAD] = "Synth Pad",
    [MPT_SYNTH_EFFECTS] = "Synth Effects",
    [MPT_ETHNIC] = "Ethnic",
    [MPT_PERCUSSIVE] = "Percussive",
    [MPT_SOUND_EFFECTS] = "Sound Effects",
    [MPT_DRUMS] = "Drums"
};


enum icon_title programTypeIcons[MPT_END] = {
    [MPT_PIANO] = T_ICON_PIANO,
    [MPT_CHROMATIC_PERCUSSION] = T_ICON_PERCUSSION,
    [MPT_ORGAN] = T_ICON_ORGAN,
    [MPT_GUITAR]  = T_ICON_GUITAR,
    [MPT_BASS]  = T_ICON_BASS,
    [MPT_STRINGS] = T_ICON_VIOLIN,
    [MPT_ENSEMBLE] = T_ICON_CONTRABASS,
    [MPT_BRASS] = T_ICON_BRASS,
    [MPT_REED] = T_ICON_BRASS,  // Or Mics
    [MPT_PIPE] = T_ICON_FLUTE,
    [MPT_SYNTH_LEAD] = T_ICON_KEYBOARD,
    [MPT_SYNTH_PAD] = T_ICON_PAD,
    [MPT_SYNTH_EFFECTS] = T_ICON_EFFECTS,
    [MPT_ETHNIC] = T_ICON_MIDI,       // Fallback for OTHER?? Idk
    [MPT_PERCUSSIVE] = T_ICON_PERCUSSION,
    [MPT_SOUND_EFFECTS] = T_ICON_EFFECTS,
    [MPT_DRUMS] = T_ICON_DRUMS
};


void oldMidiLoad() {
    FILE* fptr = fopen("assets/midi/instruments.txt", "r");
    if (!fptr) return;

    for (int i=0; i<129; i++) {
        long start = ftell(fptr), end=0;
        int j=0, c;
        while ((c=fgetc(fptr))!=EOF && c!='\n') j++;
        end=ftell(fptr);
        
        char* str = malloc((j+1)*sizeof(char));
        if (!str) continue;

        fseek(fptr, start, SEEK_SET);
        fgets(str, j+1, fptr);
        
        _midiPrograms[i].name = str;
        _midiPrograms[i].type = i/8;
        _midiPrograms[i].icon = programTypeIcons[i/8];

        fseek(fptr, end-1, SEEK_SET);
    }
}


void midiLoadInstruments() {
    FILE* fptr = fopen("assets/midi/instruments.txt", "r");
    if (!fptr) return;

    char buffer[256];
    for (int i = 0; i<129 && fgets(buffer, sizeof(buffer), fptr); i++) {
        buffer[strcspn(buffer, "\r\n")] = 0;

        char* str = malloc(strlen(buffer) + 1);
        if (!str) continue;
        
        strcpy(str, buffer);

        _midiPrograms[i].name = str;
        _midiPrograms[i].type = i / 8;
        _midiPrograms[i].icon = programTypeIcons[i / 8];
    }
    fclose(fptr);
}

void midiFreeInstruments() {
    for (int i=0; i<129; i++) {
        if (_midiPrograms[i].name) free(_midiPrograms[i].name);
        _midiPrograms[i].name = NULL;
    }
}


void midiInit() {
    midiLoadInstruments();
}


void midiClose() {
    midiFreeInstruments();
}


const char* midiGetProgramName(int program) {
    if (program<0 || program>128) return NULL;
    return _midiPrograms[program].name;
}

const char* midiProgramGetName(MidiProgram program) {
    if (!program) return NULL;
    return program->name;
}

const char* midiGetProgramTypeString(int program) {
    if (program<0 || program>128) return NULL;
    return programTypeStrings[_midiPrograms[program].type];
}

const char* midiProgramGetTypeString(MidiProgram program) {
    if (!program) return NULL;
    return programTypeStrings[program->type];
}

enum midi_program_type midiGetProgramType(int program) {
    if (program<0 || program>128) return MPT_END;
    return _midiPrograms[program].type;
}

enum midi_program_type midiProgramGetType(MidiProgram program) {
    if (!program) return MPT_END;
    return program->type;
}

enum icon_title midiGetTypeIconFromType(enum midi_program_type type) {
    if (type>=MPT_END || type<0) return T_ICON_END;
    return programTypeIcons[type];
}

enum icon_title midiGetProgramTypeIcon(int program) {
    if (program<0 || program>128) return T_ICON_END;
    return _midiPrograms[program].icon;
}

enum icon_title midiProgramGetTypeIcon(MidiProgram program) {
    if (!program) return T_ICON_END;
    return program->icon;
}

int midiProgramGetProgramNum(MidiProgram program) {
    if (!program) return 0;
    int t=(int)(program-_midiPrograms);
    if (t<0 || t>128) return 0;
    return t;
}

struct midi_programs_array midiGetProgramsByType(enum midi_program_type type) {
    struct midi_programs_array ret = {0,NULL};
    if (type>=MPT_END || type<0) return ret;

    if (type==MPT_DRUMS) {
        ret.num=1;
        MidiProgram* arr = malloc(ret.num*sizeof(MidiProgram));
        if (!arr) return ret;
        arr[0] = _midiPrograms+128;
        ret.array = arr;
        return ret;
    } else {
        ret.num=8;
        MidiProgram* arr = malloc(ret.num*sizeof(MidiProgram));
        if (!arr) return ret;
        int offset=8*type;
        for (int i=0; i<8; i++) arr[i] = _midiPrograms+i+offset;
        ret.array = arr;
        return ret;
    }   
}

void midiFreeMidiProgramArray(struct midi_programs_array* mpa) {
    if (!mpa) return;
    if (mpa->array) free(mpa->array);
    mpa->num = 0;
    mpa->array = NULL;
}