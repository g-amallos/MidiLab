#ifndef EXPORT_INTERNAL_H
#define EXPORT_INTERNAL_H

#include "../backend_internal.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <export.h>




int getTrackThemeColorIdx(int i);                       // modules/interface/render/tracks.c
void setTrackThemeColorIdx(int tracki, int themei);     // modules/interface/render/tracks.c
void updateTrackUItitle(int tracki);                    // modules/interface/render/tracks.c
void createTrackUIsFromScratch(uint8_t* colArr);        // modules/interface/render/tracks.c
void freeTrackUIs();                                    // modules/interface/render/tracks.c
void projectLoadTmpProject(ProjectData newProject);     // modules/backend/project.c




/*  os.c  */

void removeDirectory(const char* dir);




/*  export.c  */

void writeUint32(FILE* fptr, uint32_t data);
void writeUint16(FILE* fptr, uint16_t data);
void writeUint8(FILE* fptr, uint8_t data);
int readUint32(FILE* fptr, uint32_t* ret);
int readUint16(FILE* fptr, uint16_t* ret);
int readUint8(FILE* fptr, uint8_t* ret);
int readString(FILE* fptr, char* buffer, int len);
void writeString(FILE* fptr, const char* string);

int writeProject(FILE* fptr);



/*  wave.c  */

int exportProjectAsWaveForExportAll(const char* filename, int totalJobs, int currentJob);



/*  midi.c  */

enum midi_meta_type {
    MIDI_META_SEQUENCE_NUMBER=0x00,
    MIDI_META_TEXT_EVENT=0x01,
    MIDI_META_COPYRIGHT_NOTICE=0x02,
    MIDI_META_TRACK_NAME=0x03,
    MIDI_META_INSTRUMENT_NAME=0x04,
    MIDI_META_LYRIC_TEXT=0x05,
    MIDI_META_MARKER_TEXT=0x06,
    MIDI_META_END_OF_TRACK=0x2F,
    MIDI_META_TEMPO_SETTING=0x51,
    MIDI_META_TIME_SIGNATURE=0x58,
    MIDI_META_KEY_SIGNATURE=0x59
};

enum midi_channel_event {
    MIDI_CHANNEL_NOTE_OFF=0x80,
    MIDI_CHANNEL_NOTE_ON=0x90,
    MIDI_CHANNEL_CONTROL_CHANGE=0xB0,
    MIDI_CHANNEL_PROGRAM_CHANGE=0xC0,
    MIDI_CHANNEL_PITCH_WHEEL_CHANGE=0xE0
};

enum midi_controller_message {
    MIDI_CONTROLLER_MSB_MODULATION_WHEEL=0x00,
    MIDI_CONTROLLER_MSB_DATA_ENTRY=0x06,
    MIDI_CONTROLLER_MSB_CHANNEL_VOLUME=0x07,
    MIDI_CONTROLLER_MSB_PAN=0x0A,
    MIDI_CONTROLLER_MSB_EXPRESSION=0x0B,
    
    MIDI_CONTROLLER_LSB_MODULATION_WHEEL=0x21,
    MIDI_CONTROLLER_LSB_DATA_ENTRY=0x26,
    MIDI_CONTROLLER_LSB_CHANNEL_VOLUME=0x27,
    MIDI_CONTROLLER_LSB_PAN=0x2A,
    MIDI_CONTROLLER_LSB_EXPRESSION=0x2B
};


struct note_on_data {
    uint8_t key;
    uint8_t velocity;
    uint8_t channel;
    uint8_t unused;
    uint16_t trackIdx;
    uint32_t timestamp;
};

struct tracks_in_channel {
    uint8_t channel;
    uint8_t isUsed;
    uint16_t capacity;
    uint16_t size;
    uint16_t* buffer;
};

struct midi_channel_export {
    uint8_t channelsUsed;
    struct tracks_in_channel channels[16];
};

#endif