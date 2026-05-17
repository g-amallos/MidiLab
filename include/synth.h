#ifndef SYNTH_H
#define SYNTH_H

#include <stdint.h>



int synthInit();
int synthClose();

void audioInputCallback(void *buffer, unsigned int frames);


void synthPanic();
void synthAllNoteOffChannel(uint8_t channel);
void synthNoteOn(uint8_t key, float velocity, uint8_t channel);
void synthProgramNoteOn(uint8_t key, float velocity, uint8_t program);
void synthProgramNoteOnPanning(uint8_t key, float velocity, uint8_t program, float panning);




#endif