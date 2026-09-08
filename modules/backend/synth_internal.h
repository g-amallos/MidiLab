#ifndef SYNTH_INTERNAL_H
#define SYNTH_INTERNAL_H

#include <synth.h>

void exportSynthPanic();
void exportSynthProgramNoteOnPanning(uint8_t key, float velocity, uint8_t program, float panning, int track);
void exportSynthProgramNoteOffPanning(uint8_t key, uint8_t program, int track);
void renderExportAudio(void* buffer, unsigned int frames);

void exportChannelPrefix(int track, uint8_t program, float panning);
void exportProgramNoteOnFromTrack(uint8_t key, float velocity, int track);
void exportProgramNoteOffFromTrack(uint8_t key, int track);

#endif