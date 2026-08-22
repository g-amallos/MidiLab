#define TSF_IMPLEMENTATION

#include <tsf.h>
#include <stdlib.h>
#include <synth.h>
#include "backend_internal.h"
#include <backend.h>


tsf* synthSF=NULL;

int synthInit() {
    synthSF= tsf_load_filename("assets/midi/GeneralUserGSv1.471.sf2");
    tsf_set_output(synthSF, TSF_STEREO_INTERLEAVED, 44100, 0);

    return 0;
}


int synthClose() {
    if (synthSF) tsf_close(synthSF);
    synthSF=NULL;

    return 0;
}



void audioInputCallback(void *buffer, unsigned int frames) {
    tsf_render_float(synthSF, (float*)buffer, frames, 0);
}




void synthPanic() {
    if (synthSF) tsf_note_off_all(synthSF);
}

void synthAllNoteOffChannel(uint8_t channel) {
    if (synthSF) tsf_channel_note_off_all(synthSF, channel);
}

void synthNoteOn(uint8_t key, float velocity, uint8_t channel) {
    if (synthSF) tsf_channel_note_on(synthSF, channel, key, velocity);
}

void synthProgramNoteOn(uint8_t key, float velocity, uint8_t program) {
    if (synthSF) tsf_bank_note_on(synthSF, 0, program, key, velocity);
}

void synthProgramNoteOnPanning(uint8_t key, float velocity, uint8_t program, float panning) {
    if (synthSF) {
        // basically channel=program (not standard midi)
        //printf("Key=%u, Velocity=%f, Program=%u, Panning=%f\n", key, velocity, program, panning);
        tsf_channel_set_presetindex(synthSF, program, program);
        tsf_channel_set_pan(synthSF, program, panning);
        tsf_channel_note_on(synthSF, program, key, velocity);
    }
}

void synthProgramNoteOffPanning(uint8_t key, uint8_t program) {
    if (synthSF) {
        tsf_channel_set_presetindex(synthSF, program, program);
        tsf_channel_note_on(synthSF, program, key, 0);
    }
}



void synthExecuteEvent(MidiEvent event) {
    if (!event) return;
    
    switch (event->type) {
        case MM_NOTE_ON: {
            synthProgramNoteOnPanning(event->note_on.key, 0.007874*event->note_on.velocity, event->channel, 0.5);
            break;
        };

        case MM_NOTE_OFF: {
            //printf("Note Off executed\n");
            synthProgramNoteOffPanning(event->note_off.key, event->channel);
            break;
        };

        default: return;
    }
}