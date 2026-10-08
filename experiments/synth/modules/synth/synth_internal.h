#ifndef CUSTOM_SYNTH_INTERNAL_H
#define CUSTOM_SYNTH_INTERNAL_H

#include <synth.h>
#include <vector.h>



enum synth_voice_state {
    SVS_ENTER,
    SVS_HOLD,
    SVS_EXIT
};


enum voice_harmonics {
    HARMONICS_SINE,
    HARMONICS_PIANO,
    HARMONICS_GUITAR,
    HARMONICS_ELECTRIC_GUITAR,

    HARMONICS_BASS,
    HARMONICS_CELLO,
    HARMONICS_CHURCH_ORGAN,

    



    HARMONICS_END
};


enum voice_envelopes {
    ENVELOPE_EXPONENTIAL,
    ENVELOPE_HOLD,
    ENVELOPE_HOLD_TRIGONOMETRIC,
    ENVELOPE_LINEAR,
    ENVELOPE_CUBIC
};


struct oscillator {
    float amplitude;
    float frequency;
};

struct envelope_adsr {
    float attackSec;
    float decaySec;
    float sustainLevel;
    float releaseSec;    
};


struct voice_timbre {
    enum instruments instrument;
    enum voice_harmonics harmonics;     // ID for the harmonics-amplitude relations
    enum voice_envelopes envelope;      // ID for the shape of the volume
    struct envelope_adsr adsr; 
    struct oscillator oscillator;
};



struct synth_voice {
    struct voice_timbre timbre;

    float pitch;
    float frequency;
    float volume;

    uint8_t originalKey;
    uint8_t channel;

    uint8_t voiceState;
    uint8_t voiceOn;
    
    double phase;
    double time;
    double offTime;
};


struct synth_channel {
    int channel;
    float pitch;

    uint32_t activeVoices;
    Vector voices;
};

struct synth_data {
    uint32_t sampleRate;
    double dt;

    int numOfChannels;
    float dampen;
    uint32_t activeVoices;
    struct synth_channel* channels;
};





void voiceDestroy(SynthVoice voice);
SynthVoice _createVoice(int key, float volume, enum instruments instrument);
float _voiceStepFrame(SynthVoice voice, double dt, float channelPitch, int* shouldDelete);
struct voice_timbre getVoiceTimbreForInstrument(enum instruments instrument);
float getVolumeForEnvelope(SynthVoice sv);
float getNormalizedSampleForVoice(SynthVoice sv);
void synthDeleteVoice(Synth synth, SynthVoice voice);

#endif