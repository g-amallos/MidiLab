#ifndef SYNTH_H
#define SYNTH_H

#include <stdint.h>


typedef struct midi_event* MidiEvent;


int synthInit();
int synthClose();

void audioInputCallback(void *buffer, unsigned int frames);


void synthPanic();
void synthAllNoteOffChannel(uint8_t channel);
void synthNoteOn(uint8_t key, float velocity, uint8_t channel);
void synthProgramNoteOn(uint8_t key, float velocity, uint8_t program);
void synthProgramNoteOnPanning(uint8_t key, float velocity, uint8_t program, float panning);
void synthProgramNoteOffPanning(uint8_t key, uint8_t program);


void synthExecuteEvent(MidiEvent event);



#endif