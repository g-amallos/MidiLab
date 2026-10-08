#include "synth_internal.h"
#include <vector.h>
#include <stdlib.h>
#include <stdio.h>



static int targetNoteOnKey=0;




static void _voiceDestroy(void* voice) {
    voiceDestroy((SynthVoice)voice);
}


static void closeSynthChannel(struct synth_channel* synthChannel) {
    if (!synthChannel) return;
    vectorDestroy(synthChannel->voices);
    synthChannel->voices = NULL;
}

void synthClose(Synth synth) {
    if (!synth) return;
    
    int chans = synth->numOfChannels;
    for (int i=0; i<chans; i++) {
        closeSynthChannel(synth->channels+i);
    }

    free(synth->channels);
    free(synth);
}


struct synth_channel initSynthChannel(int channel) {
    struct synth_channel ret = {
        .channel = channel,
        .pitch = 0.0,
        .activeVoices = 0,
        .voices = vectorCreate(_voiceDestroy)
    };
    return ret;
}


Synth synthCreate(int sampleRate, int channels) {
    if (sampleRate<100 || channels<1 || channels>128) return NULL;

    Synth ret = malloc(sizeof(struct synth_data));
    if (!ret) return NULL;

    ret->sampleRate = (uint32_t)sampleRate;
    ret->dt = 1.0/(double)sampleRate;
    ret->activeVoices = 0;
    ret->numOfChannels = channels;
    ret->dampen = 0.35/channels;
    ret->channels = malloc(channels*sizeof(struct synth_channel));
    
    if (!(ret->channels)) {
        synthClose(ret);
        return NULL;
    }

    int shouldDelete = 0;
    for (int i=0; i<channels; i++) {
        (ret->channels)[i] = initSynthChannel(i);
        shouldDelete |= !((ret->channels)[i].voices);
    }
    
    if (shouldDelete) {
        synthClose(ret);
        return NULL;
    }
    

    return ret;
}






SynthVoice synthVoiceCreate(Synth synth, int channel, int key, float volume, enum instruments instrument) {
    if (!synth || channel<0 || channel>=synth->numOfChannels) return NULL;

    SynthVoice sv = _createVoice(key, volume, instrument);
    if (!sv) return NULL;
    if (vectorAppend((synth->channels)[channel].voices, sv)) {
        voiceDestroy(sv);
        return NULL;
    }
    return sv;
}



static int _helperFindNoteOn(void* pointer) {
    if (!pointer) return 0;
    SynthVoice voice = (SynthVoice)pointer;
    return (voice->voiceOn)&&(voice->originalKey==targetNoteOnKey);
}

static void _helperCloseNote(void* pointer) {
    if (!pointer) return;
    SynthVoice voice = (SynthVoice)pointer;
    voice->voiceOn = 0;
}




SynthVoice synthNoteOn(Synth synth, int channel, int key, float volume, enum instruments instrument) {
    if (!synth || channel<0 || channel>=synth->numOfChannels) return NULL;
    if (volume<=0.0) {
        synthNoteOff(synth, channel, key);
        return NULL;
    }

    if (volume>1.0) volume=1.0;

    SynthVoice sv = _createVoice(key, volume, instrument);
    if (!sv) return NULL;
    
    Vector vec = (synth->channels)[channel].voices;
    targetNoteOnKey = key;
    vectorApplyFunction(vec, _helperFindNoteOn, _helperCloseNote);

    if (vectorAppend(vec, sv)) {
        voiceDestroy(sv);
        return NULL;
    }
    
    return sv;
}


void synthNoteOff(Synth synth, int channel, int key) {
    if (!synth || channel<0 || channel>=synth->numOfChannels) return;
    targetNoteOnKey = key;
    vectorApplyFunction((synth->channels)[channel].voices, _helperFindNoteOn, _helperCloseNote);
}

void synthNoteOffAllChannel(Synth synth, int channel) {
    if (!synth || channel<0 || channel>=synth->numOfChannels) return;
    vectorApplyFunctionForAllElements((synth->channels)[channel].voices, _helperCloseNote);
}

void synthPanic(Synth synth) {
    if (!synth) return;
    int ch = synth->numOfChannels;
    for (int i=0; i<ch; i++) vectorApplyFunctionForAllElements((synth->channels)[i].voices, _helperCloseNote);
}

void synthPanicNotGracefully(Synth synth) {
    if (!synth) return;
    int ch = synth->numOfChannels;
    for (int i=0; i<ch; i++) vectorDeleteAllElements((synth->channels)[i].voices);
}


static void _synthGenerateSingleFloatSample(Synth synth, float* target) {
    if (!synth || !target) return;
    float sample=0.0;
    double dt = synth->dt;
    int chans = synth->numOfChannels;
    int shouldDelete=0;

    for (int i=0; i<chans; i++) {
        Vector voices = (synth->channels)[i].voices;
        
        uint32_t n=vectorGetSize(voices), i=0;
        float channelPitch=(synth->channels)[i].pitch;
        while (i<n) {
            sample += _voiceStepFrame(vectorGetAt(voices, i), dt, channelPitch, &shouldDelete);
            if (shouldDelete) {
                shouldDelete=0;
                vectorDeleteAt(voices, i);
                n--;
            } else i++;
        }
        *target = sample*synth->dampen;
    }
}


void synthGenerateWave(Synth synth, void* buffer, uint32_t samples, enum audio_sample sampleType, int channels) {
    if (!synth || !buffer || !samples || (channels<1 || channels>1) || sampleType!=AUDIO_FLOAT) return;

    for (uint32_t i=0; i<samples; i++) {
        _synthGenerateSingleFloatSample(synth, ((float*)buffer)+i);
    }
}


void synthDeleteVoice(Synth synth, SynthVoice voice) {
    if (!synth || !voice || (voice->channel)>=synth->numOfChannels) return;
    vectorDeleteElement((synth->channels)[voice->channel].voices, voice);
}