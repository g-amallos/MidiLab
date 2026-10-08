#ifndef CUSTOM_SYNTH_H
#define CUSTOM_SYNTH_H

#include <stdint.h>

typedef struct synth_voice *SynthVoice;
typedef struct synth_data *Synth;


enum instruments {
    INSTRUMENT_SINE,

    INSTRUMENT_PIANO,
    INSTRUMENT_GUITAR,
    INSTRUMENT_ELECTRIC_GUITAR,

    INSTRUMENT_BASS,
    INSTRUMENT_CELLO,
    INSTRUMENT_CHURCH_ORGAN,
    INSTRUMENT_STADIUM_ORGAN,




    INSTRUMENT_TEST_1,


    INSTRUMENT_END
};




enum audio_sample {
    AUDIO_FLOAT,
    AUDIO_UI8,
    AUDIO_UI16
};





Synth synthCreate(int sampleRate, int channels);
void synthClose(Synth synth);

SynthVoice synthVoiceCreate(Synth synth, int channel, int key, float volume, enum instruments instrument);
void synthVoiceClose(SynthVoice voice);
SynthVoice synthNoteOn(Synth synth, int channel, int key, float volume, enum instruments instrument);
void synthNoteOff(Synth synth, int channel, int key);
void synthGenerateWave(Synth synth, void* buffer, uint32_t samples, enum audio_sample sampleType, int channels);
void synthVoiceChangePitch(SynthVoice voice, float newPitch);
void synthNoteOffAllChannel(Synth synth, int channel);
void synthPanic(Synth synth);
void synthPanicNotGracefully(Synth synth);

#endif