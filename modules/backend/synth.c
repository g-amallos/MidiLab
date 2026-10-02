//#define FLUIDSYNTH_NOT_A_DLL

#include <stdlib.h>
#include <synth.h>
#include "backend_internal.h"
#include <backend.h>
#include <fluidsynth.h>


static fluid_settings_t* synthSettings = NULL;
static fluid_synth_t* synthSF = NULL;

static fluid_settings_t* exportSettings = NULL;
static fluid_synth_t* exportSF = NULL;

static int synthFontId = -1;
static int exportFontId = -1;


int synthInit() {
    synthSettings = new_fluid_settings();
    fluid_settings_setstr(synthSettings, "audio.driver", "none"); // Disable internal audio output drivers
    fluid_settings_setint(synthSettings, "synth.midi-channels", 128);
    fluid_settings_setnum(synthSettings, "synth.gain", 0.8);
    fluid_settings_setnum(synthSettings, "synth.reverb.damp", 0.3);
    fluid_settings_setnum(synthSettings, "synth.reverb.level", 0.7);
    fluid_settings_setnum(synthSettings, "synth.reverb.room-size", 0.5);
    fluid_settings_setnum(synthSettings, "synth.reverb.width", 0.8);
    fluid_settings_setnum(synthSettings, "synth.chorus.depth", 3.6);
    fluid_settings_setnum(synthSettings, "synth.chorus.level", 0.55);
    fluid_settings_setint(synthSettings, "synth.chorus.nr", 4);
    fluid_settings_setnum(synthSettings, "synth.chorus.speed", 0.36);
    synthSF = new_fluid_synth(synthSettings);
    synthFontId = fluid_synth_sfload(synthSF, "assets/midi/GeneralUserGSv2.0.3.sf2", 1);

    exportSettings = new_fluid_settings();
    fluid_settings_setstr(exportSettings, "audio.driver", "none");
    fluid_settings_setint(exportSettings, "synth.midi-channels", 128);
    fluid_settings_setnum(exportSettings, "synth.gain", 0.8);
    fluid_settings_setnum(exportSettings, "synth.reverb.damp", 0.3);
    fluid_settings_setnum(exportSettings, "synth.reverb.level", 0.7);
    fluid_settings_setnum(exportSettings, "synth.reverb.room-size", 0.5);
    fluid_settings_setnum(exportSettings, "synth.reverb.width", 0.8);
    fluid_settings_setnum(exportSettings, "synth.chorus.depth", 3.6);
    fluid_settings_setnum(exportSettings, "synth.chorus.level", 0.55);
    fluid_settings_setint(exportSettings, "synth.chorus.nr", 4);
    fluid_settings_setnum(exportSettings, "synth.chorus.speed", 0.36);
    exportSF = new_fluid_synth(exportSettings);
    exportFontId = fluid_synth_sfload(exportSF, "assets/midi/GeneralUserGSv2.0.3.sf2", 1);

    return 0;
}


int synthClose() {
    if (synthSF) {
        if (synthFontId!=-1) fluid_synth_sfunload(synthSF, synthFontId, 1);
        delete_fluid_synth(synthSF);
    }
    if (synthSettings) delete_fluid_settings(synthSettings);
    
    if (exportSF) {
        if (exportFontId!=-1) fluid_synth_sfunload(exportSF, exportFontId, 1);
        delete_fluid_synth(exportSF);
    }
    if (exportSettings) delete_fluid_settings(exportSettings);

    synthSF = NULL;
    exportSF = NULL;
    synthSettings = NULL;
    exportSettings = NULL;
    synthFontId = -1;
    exportFontId = -1;

    return 0;
}


void recordAudioFrames(float* buffer, int samples);          // backend/fft.c

void audioInputCallback(void *buffer, unsigned int frames) {
    if (synthSF) {
        fluid_synth_write_float(synthSF, frames, buffer, 0, 2, buffer, 1, 2);
        recordAudioFrames(buffer, (int)frames);
    }
}

void renderExportAudio(void* buffer, unsigned int frames) {
    if (exportSF) {
        fluid_synth_write_s16(exportSF, frames, buffer, 0, 2, buffer, 1, 2);
    }
}


void synthPanic() {
    if (synthSF) {
        midiActionRemoveAll();
        fluid_synth_system_reset(synthSF);
        globalHandlerUpdateAllTracks();
    }
}

void exportSynthPanic() {
    if (exportSF) {
        fluid_synth_system_reset(exportSF);
        _exportSetupSynthTracks();
    }
}

void synthAllNoteOffChannel(uint8_t channel) {
    if (synthSF) fluid_synth_all_notes_off(synthSF, channel);
}

void synthNoteOn(uint8_t key, float velocity, uint8_t channel) {
    if (synthSF) {
        fluid_synth_noteon(synthSF, channel, key, (int)(velocity * 127.0f));
    }
}

void synthProgramNoteOn(uint8_t key, float velocity, uint8_t program) {
    if (synthSF) {
        fluid_synth_program_change(synthSF, 0, program);
        fluid_synth_noteon(synthSF, 0, key, (int)(velocity * 127.0f));
    }
}

void synthProgramNoteOnPanning(uint8_t key, float velocity, uint8_t program, float panning, int track) {
    if (synthSF) {
        int vel_int = (int)(velocity * 127.0f);
        int pan_int = (int)(panning * 127.0f);
        
        int is_drum = (midiGetProgramType(program) == MPT_DRUMS);
        int bank = is_drum ? 128 : 0;

        fluid_synth_bank_select(synthSF, track, bank);
        fluid_synth_program_change(synthSF, track, program);
        fluid_synth_cc(synthSF, track, 10, pan_int);
        
        fluid_synth_noteon(synthSF, track, key, vel_int);
    }
}

void synthChannelPrefix(int track, uint8_t program, float panning) {
    if (synthSF) {
        int isDrum = (midiGetProgramType(program) == MPT_DRUMS);
        fluid_synth_set_channel_type(synthSF, track, isDrum);
        fluid_synth_bank_select(synthSF, track, isDrum*128);
        fluid_synth_program_change(synthSF, track, program*(!isDrum));
        fluid_synth_cc(synthSF, track, 10, (int)(panning*127.0));
        //printf("synthChannelPrefix: track: %u, program: %u, panning: %.2f\n", track, program, panning);
    }
}

void exportChannelPrefix(int track, uint8_t program, float panning) {
    if (exportSF) {
        int isDrum = (midiGetProgramType(program) == MPT_DRUMS);
        fluid_synth_set_channel_type(exportSF, track, isDrum);
        fluid_synth_bank_select(exportSF, track, isDrum*128);
        fluid_synth_program_change(exportSF, track, program*(!isDrum));
        fluid_synth_cc(exportSF, track, 10, (int)(panning*127.0));
    }
}

void synthProgramNoteOnFromTrack(uint8_t key, float velocity, int track) {
    if (synthSF) {
        fluid_synth_noteon(synthSF, track, key, (int)(velocity*127.0f));
    }
}

void synthProgramNoteOffFromTrack(uint8_t key, int track) {
    if (synthSF) {
        fluid_synth_noteoff(synthSF, track, key);
    }
}

void exportSynthProgramNoteOnPanning(uint8_t key, float velocity, uint8_t program, float panning, int track) {
    if (exportSF) {
        int isDrum = (midiGetProgramType(program) == MPT_DRUMS);

        fluid_synth_bank_select(exportSF, track, isDrum*128);
        fluid_synth_program_change(exportSF, track, program);
        fluid_synth_cc(exportSF, track, 10, (int)(panning * 127.0f));
        fluid_synth_noteon(exportSF, track, key, (int)(velocity * 127.0f));
    }
}

void synthProgramNoteOffPanning(uint8_t key, uint8_t program, int track) {
    (void)program;
    if (synthSF) {
        fluid_synth_noteoff(synthSF, track, key);
    }
}

void exportSynthProgramNoteOffPanning(uint8_t key, uint8_t program, int track) {
    (void)program;
    if (exportSF) {
        fluid_synth_noteoff(exportSF, track, key);
    }
}

void exportProgramNoteOnFromTrack(uint8_t key, float velocity, int track) {
    if (exportSF) {
        fluid_synth_noteon(exportSF, track, key, (int)(velocity*127.0f));
    }
}

void exportProgramNoteOffFromTrack(uint8_t key, int track) {
    if (exportSF) {
        fluid_synth_noteoff(exportSF, track, key);
    }
}


void synthExecuteEvent(MidiEvent event) {
    if (!event) return;
    
    switch (event->type) {
        case MM_NOTE_ON: {
            //s ynthProgramNoteOnPanning(event->note_on.key, 0.007874*event->note_on.velocity, event->channel, );
            synthProgramNoteOnFromTrack(event->note_on.key, 0.007874*event->note_on.velocity, event->track);
            break;
        };

        case MM_NOTE_OFF: {
            //printf("Note Off executed\n");
            synthProgramNoteOffFromTrack(event->note_off.key, event->track);
            break;
        };

        default: return;
    }
}