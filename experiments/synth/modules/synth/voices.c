#include "synth_internal.h"
#include <stdlib.h>
#include <math.h>

#include <stdio.h>


#define M_TAU 6.283185307179586476925286766559


static float _frequencyFromKey(float key) {
    return 440.0*powf(2.0, (key-69.0)/12.0);
}




void voiceDestroy(SynthVoice voice) {
    if (!voice) return;
    free(voice);
}



SynthVoice _createVoice(int key, float volume, enum instruments instrument) {
    SynthVoice sv = malloc(sizeof(struct synth_voice));
    if (!sv) return NULL;

    sv->timbre = getVoiceTimbreForInstrument(instrument);
    sv->pitch = key;
    sv->frequency = _frequencyFromKey(key);
    sv->volume = volume;
    sv->originalKey = key;
    sv->channel = 0;
    sv->voiceState = SVS_ENTER;
    sv->voiceOn = 1;
    sv->phase = 0;
    sv->time = 0;
    sv->offTime = -1;

    return sv;
}

void synthVoiceChangePitch(SynthVoice voice, float newPitch) {
    if (!voice) return;
    voice->pitch = newPitch;
    voice->frequency = _frequencyFromKey(newPitch);
}

void synthVoiceClose(SynthVoice voice) {
    if (!voice) return;
    voice->voiceOn = 0;
}


static int _voiceShouldBeDeleted(SynthVoice sv) {
    if (!sv) return 1;
    if (sv->voiceState==SVS_EXIT && !(sv->voiceOn) && sv->offTime>0 && sv->time-sv->offTime>=sv->timbre.adsr.releaseSec) return 1;
    else return 0;
}

static void _voiceUpdateFlags(SynthVoice sv) {
    if (!sv) return;

    if (sv->voiceOn) {
        
    } else {
        if (sv->offTime<0) sv->offTime = sv->time;
    }

    if (sv->time<sv->timbre.adsr.attackSec) sv->voiceState=SVS_ENTER;
    else if (sv->offTime<0) sv->voiceState=SVS_HOLD;
    else sv->voiceState=SVS_EXIT;
}

static void _voiceUpdateOscillator(SynthVoice sv, double dt, float channelPitch) {
    if (!sv) return;

    struct oscillator osc = sv->timbre.oscillator;
    if (osc.amplitude!=0.0 && osc.frequency!=0.0) sv->phase += _frequencyFromKey(sv->pitch+channelPitch+osc.amplitude*sinf(M_TAU*osc.frequency*sv->time))*dt*M_TAU;
    else sv->phase += sv->frequency*dt*M_TAU;
    sv->time += dt;
}


float _voiceStepFrame(SynthVoice voice, double dt, float channelPitch, int* shouldDelete) {
    if (!voice) return 0.0;

    _voiceUpdateFlags(voice);
    if (_voiceShouldBeDeleted(voice)) {
        *shouldDelete = 1;
        return 0.0;
    }

    double volume = getVolumeForEnvelope(voice);
    double sample = getNormalizedSampleForVoice(voice);
    

    //printf("Voice: `%s`, %.5f, %.5f, %.5f, %.3f, %.3f, %d, %d\n", lol, voice->volume, volume, sample, voice->time, voice->offTime, voice->voiceOn, voice->voiceState);
    

    sample = voice->volume*volume*sample;
    
    _voiceUpdateOscillator(voice, dt, channelPitch);
    
    return sample;
}