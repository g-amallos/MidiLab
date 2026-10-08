#include <stdio.h>
#include <wave.h>
#include <synth.h>
#include <utils.h>


#define BUFFER_SIZE 4096

float buffer[BUFFER_SIZE] = {0,};
Synth synth = NULL;
WaveHeader wh = NULL;



void writeDuration(double dur) {
    uint32_t samps = (dur*44100.0);
    while (samps>=BUFFER_SIZE) {
        synthGenerateWave(synth, buffer, BUFFER_SIZE, AUDIO_FLOAT, 1);
        waveFileAddSamples(wh, buffer, BUFFER_SIZE, MONO_FLOAT);
        samps -= BUFFER_SIZE;
    }
    if (samps) {
        synthGenerateWave(synth, buffer, samps, AUDIO_FLOAT, 1);
        waveFileAddSamples(wh, buffer, samps, MONO_FLOAT);
    }
}


int main(int argc, char** argv) {
    if (argc<2) {
        fprintf(stderr, "Usage: `./synth test.wav`\n");
        return 1;
    }

    wh = createWaveFile(argv[1], 44100, 1, 16);

    if (!wh) {
        fprintf(stderr, "Couldn't open `%s` correctly\n", argv[1]);
        return 2;
    }


    synth = synthCreate(44100, 1);

    
    writeDuration(0.5);

    double chordDur=3.0, transitionDur=1.0;
    int notes[][3] = {{55,60,64}, {56,59,64}, {57,60,65}, {56,60,65}, {55,60,64}};
    int s = 5;


    enum instruments instr = INSTRUMENT_STADIUM_ORGAN;

    SynthVoice sv1 = synthVoiceCreate(synth, 0, 55, 1.0, instr);
    SynthVoice sv2 = synthVoiceCreate(synth, 0, 60, 1.0, instr);
    SynthVoice sv3 = synthVoiceCreate(synth, 0, 64, 1.0, instr);

    SynthVoice svs[] = {sv1, sv2, sv3};

    for (int i=0; i<s; i++) {
        synthVoiceChangePitch(sv1, notes[i][0]);
        synthVoiceChangePitch(sv2, notes[i][1]);
        synthVoiceChangePitch(sv3, notes[i][2]);

        double duration=chordDur;
        writeDuration(duration-transitionDur);

        if (i<s-1) {
            int quality = 200;
            double stepTime = transitionDur/quality;
            for (int j=0; j<quality; j++) {
                for (int k=0; k<3; k++) {
                    synthVoiceChangePitch(svs[k], lerpFloat(notes[i][k], notes[i+1][k], j/(double)quality));
                }
                writeDuration(stepTime);
            }

        } else writeDuration(transitionDur);
    }

    synthNoteOffAllChannel(synth, 0);

    writeDuration(0.5);
   
    finishWaveFile(wh);
    wh = NULL;

    synthClose(synth);
    synth = NULL;

    return 0;
}