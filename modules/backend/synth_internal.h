#ifndef SYNTH_INTERNAL_H
#define SYNTH_INTERNAL_H

#include <synth.h>

void exportSynthPanic();
void exportSynthProgramNoteOnPanning(uint8_t key, float velocity, uint8_t program, float panning);
void exportSynthProgramNoteOffPanning(uint8_t key, uint8_t program);
void renderExportAudio(void* buffer, unsigned int frames);


#endif