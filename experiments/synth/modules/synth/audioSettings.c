#include "synth_internal.h"
#include <stdlib.h>
#include <math.h>
#include <utils.h>

#include <stdio.h>



typedef float (*HarmonicsFunction)(double phase);


static struct voice_timbre instrumentTimbres[INSTRUMENT_END+1] = {
    [INSTRUMENT_SINE] = {
        .instrument = INSTRUMENT_SINE,
        .harmonics = HARMONICS_SINE,
        .envelope = ENVELOPE_HOLD,
        .adsr = {
            .attackSec = 0.2,
            .decaySec = 0.0,
            .sustainLevel = 1.0,
            .releaseSec = 0.2
        },
        .oscillator = {
            .amplitude = 0.0,
            .frequency = 0.0
        }
    },

    [INSTRUMENT_PIANO] = {
        .instrument = INSTRUMENT_PIANO,
        .harmonics = HARMONICS_PIANO,
        .envelope = ENVELOPE_EXPONENTIAL,
        .adsr = {
            .attackSec = 0.0,
            .decaySec = 2.0,
            .sustainLevel = 0.035,
            .releaseSec = 0.1
        },
        .oscillator = {
            .amplitude = 0.0,
            .frequency = 0.0
        }
    },

    [INSTRUMENT_GUITAR] = {
        .instrument = INSTRUMENT_GUITAR,
        .harmonics = HARMONICS_GUITAR,
        .envelope = ENVELOPE_EXPONENTIAL,
        .adsr = {
            .attackSec = 0.0,
            .decaySec = 4.5,
            .sustainLevel = 0.012,
            .releaseSec = 0.08
        },
        .oscillator = {
            .amplitude = 0.02,
            .frequency = 5.0
        }
    },

    [INSTRUMENT_ELECTRIC_GUITAR] = {
        .instrument = INSTRUMENT_ELECTRIC_GUITAR,
        .harmonics = HARMONICS_ELECTRIC_GUITAR,
        .envelope = ENVELOPE_EXPONENTIAL,
        .adsr = {
            .attackSec = 0.0,
            .decaySec = 2.0,
            .sustainLevel = 0.05,
            .releaseSec = 0.1
        },
        .oscillator = {
            .amplitude = 0.01,
            .frequency = 8.0
        }
    },

    [INSTRUMENT_BASS] = {
        .instrument = INSTRUMENT_BASS,
        .harmonics = HARMONICS_BASS,
        .envelope = ENVELOPE_EXPONENTIAL,
        .adsr = {
            .attackSec = 0.0,
            .decaySec = 2.5,
            .sustainLevel = 0.08,
            .releaseSec = 0.1
        },
        .oscillator = {
            .amplitude = 0.0,
            .frequency = 0.0
        }
    },

    [INSTRUMENT_CHURCH_ORGAN] = {
        .instrument = INSTRUMENT_CHURCH_ORGAN,
        .harmonics = HARMONICS_CHURCH_ORGAN,
        .envelope = ENVELOPE_HOLD,
        .adsr = {
            .attackSec = 0.2,
            .decaySec = 0.0,
            .sustainLevel = 1.0,
            .releaseSec = 0.2
        },
        .oscillator = {
            .amplitude = 0.0,
            .frequency = 6.0
        }
    },

    [INSTRUMENT_STADIUM_ORGAN] = {
        .instrument = INSTRUMENT_STADIUM_ORGAN,
        .harmonics = HARMONICS_CHURCH_ORGAN,
        .envelope = ENVELOPE_HOLD,
        .adsr = {
            .attackSec = 0.18,
            .decaySec = 0.0,
            .sustainLevel = 1.0,
            .releaseSec = 0.18
        },
        .oscillator = {
            .amplitude = 0.15,
            .frequency = 8.0
        }
    },
    
    [INSTRUMENT_CELLO] = {
        .instrument = INSTRUMENT_CELLO,
        .harmonics = HARMONICS_CELLO,
        .envelope = ENVELOPE_HOLD,
        .adsr = {
            .attackSec = 0.18,
            .decaySec = 0.0,
            .sustainLevel = 1.0,
            .releaseSec = 0.18
        },
        .oscillator = {
            .amplitude = 0.06,
            .frequency = 5.0
        }
    },

    [INSTRUMENT_VIOLIN] = {
        .instrument = INSTRUMENT_VIOLIN,
        .harmonics = HARMONICS_VIOLIN,
        .envelope = ENVELOPE_HOLD,
        .adsr = {
            .attackSec = 0.18,
            .decaySec = 0.0,
            .sustainLevel = 1.0,
            .releaseSec = 0.18
        },
        .oscillator = {
            .amplitude = 0.06,
            .frequency = 5.0
        }
    },




    [INSTRUMENT_TEST_1] = {
        .instrument = INSTRUMENT_TEST_1,
        .harmonics = HARMONICS_ELECTRIC_GUITAR,
        .envelope = ENVELOPE_HOLD,
        .adsr = {
            .attackSec = 0.2,
            .decaySec = 0.0,
            .sustainLevel = 1.0,
            .releaseSec = 0.2
        },
        .oscillator = {
            .amplitude = 0.05,
            .frequency = 6.0
        }
    },


    [INSTRUMENT_END] = {
        .instrument = INSTRUMENT_END,
        .harmonics = HARMONICS_SINE,
        .envelope = ENVELOPE_HOLD,
        .adsr = {
            .attackSec = 0.0,
            .decaySec = 0.0,
            .sustainLevel = 0.0,
            .releaseSec = 0.0
        },
        .oscillator = {
            .amplitude = 0.0,
            .frequency = 0.0
        }
    },
};




static float _harmonicFSine(double phase) {
    return sin(phase);
}

//static float waveSampleInstrument(double phase) {
//	double freqs[26] = {1, 0.5, 0.489391, 3.51138, 0.513861, 1.49831, 3.56359, 1.01549, 2.99661, 2, 0.978782, 2.51984, 4.48985, 3.49915, 1.48041, 4.55134, 5.03968, 0.471937, 4, 6.10515, 1.02772, 1.51711, 5.57906, 6.09292, 1.03996, 6.63125};
//	double amps[26] = {0.257754, 0.103848, 0.0715495, 0.0454537, 0.0381199, 0.077318, 0.0432335, 0.0293729, 0.0629388, 0.0621578, 0.0213282, 0.0398725, 0.0287097, 0.0127814, 0.0108702, 0.00951266, 0.018656, 0.00802399, 0.0165244, 0.00811223, 0.00604897, 0.00654045, 0.00592177, 0.0060496, 0.00447863, 0.00482252};
//	double ret = 0.0;
//	for (int i=0; i<26; i++) ret+=amps[i]*sin(phase*freqs[i]);
//	return ret;
//}


static float _harmonicFPiano(double phase) {
	double freqs[19] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, 20};
	double amps[19] = {0.296269, 0.533535, 0.0976245, 0.0536083, 0.00962514, 0.00163234, 0.00244327, 0.000454291, 0.00420131, 0.000172415, 0.00014021, 0.000111968, 5.07029e-05, 2.7403e-05, 2.68666e-05, 2.77692e-05, 1.13447e-05, 1.10509e-05, 1.72898e-05};
	double ret = 0.0;
	for (int i=0; i<19; i++) ret+=amps[i]*sin(phase*freqs[i]);
	return ret;
}

static float _harmonicFPianoLow(double phase) {
	double freqs[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	double amps[20] = {0.232055, 0.300533, 0.141092, 0.106463, 0.0555311, 0.0405184, 0.0738302, 0.00291724, 0.0214992, 0.0114094, 0.00817562, 0.00288767, 0.00126523, 0.000347508, 0.000728352, 0.000321556, 0.000210449, 8.53207e-05, 6.23636e-05, 6.76665e-05};
	double ret = 0.0;
	for (int i=0; i<20; i++) ret+=amps[i]*sin(phase*freqs[i]);
	return ret;
}

static float _harmonicFGuitarClean(double phase) {
	double freqs[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	double amps[20] = {0.0999131, 0.458905, 0.0604197, 0.119207, 0.127092, 0.00941475, 0.0212868, 0.00861088, 0.064984, 0.00715678, 0.0134237, 0.00372185, 0.00167334, 0.0020608, 0.000536152, 0.000299006, 0.000693584, 0.000443355, 0.00012579, 3.17134e-05};
	double ret = 0.0;
	for (int i=0; i<20; i++) ret+=amps[i]*sin(phase*freqs[i]);
	return ret;
}

static float _harmonicFGuitarNylon(double phase) {
	double freqs[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	double amps[20] = {0.39227, 0.286018, 0.200127, 0.0713304, 0.0178324, 0.0216666, 0.00549764, 0.00143836, 0.00106106, 0.00101207, 0.000653605, 0.000149609, 0.000562611, 0.0001665, 4.38689e-05, 4.08861e-05, 2.22465e-05, 5.01954e-05, 2.07588e-05, 3.55042e-05};
	double ret = 0.0;
	for (int i=0; i<20; i++) ret+=amps[i]*sin(phase*freqs[i]);
	return ret;
}

static float _harmonicFElectricGuitar(double phase) {
	double freqs[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	double amps[20] = {0.407887, 0.214491, 0.0334368, 0.0532922, 0.15514, 0.0396138, 0.0402231, 0.0319933, 0.00977819, 0.0054772, 0.00403058, 0.00183879, 0.00161021, 0.0007245, 0.000269069, 5.54496e-05, 2.74076e-05, 2.37588e-05, 4.0486e-05, 4.68295e-05};
	double ret = 0.0;
	for (int i=0; i<20; i++) ret+=amps[i]*sin(phase*freqs[i]);
	return ret;
}

static float _harmonicFChurchOrgan(double phase) {
	double freqs[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	double amps[20] = {0.161009, 0.18194, 0.151337, 0.189113, 0.00986168, 0.0359957, 0.00530148, 0.169444, 0.00935529, 0.0117286, 0.00191257, 0.0308578, 0.00129378, 0.0069754, 0.00115615, 0.0145524, 0.000827077, 0.00449218, 0.000735, 0.0121131};
	double ret = 0.0;
	for (int i=0; i<20; i++) ret+=amps[i]*sin(phase*freqs[i]);
	return ret;
}

static float _harmonicFCello(double phase) {
	double freqs[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	double amps[20] = {0.187426, 0.247498, 0.123283, 0.171675, 0.0739558, 0.0751591, 0.04834, 0.0203921, 0.0142524, 0.00851856, 0.00667842, 0.0048311, 0.00575353, 0.00293952, 0.00266024, 0.00240422, 0.00159438, 0.00118527, 0.000772001, 0.000682601};
	double ret = 0.0;
	for (int i=0; i<20; i++) ret+=amps[i]*sin(phase*freqs[i]);
	return ret;
}

static float _harmonicFViolin(double phase) {
	double freqs[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	double amps[20] = {0.496741, 0.121256, 0.151832, 0.0396318, 0.0159512, 0.0744311, 0.0347513, 0.018468, 0.0192381, 0.0158376, 0.00389624, 0.000659305, 0.00130275, 0.00272435, 0.00108522, 0.000979415, 0.000493194, 0.000289477, 0.000178744, 0.000253104};
	double ret = 0.0;
	for (int i=0; i<20; i++) ret+=amps[i]*sin(phase*freqs[i]);
	return ret;
}

static float _harmonicFBass(double phase) {
	double freqs[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	double amps[20] = {0.832238, 0.143986, 0.015981, 0.00446496, 0.00178172, 0.000494009, 0.000142889, 0.000100442, 4.69918e-05, 3.81212e-05, 6.31625e-05, 6.20847e-05, 6.05155e-05, 5.57969e-05, 8.43725e-05, 8.31274e-05, 4.23669e-05, 8.45993e-05, 9.46398e-05, 9.47136e-05};
	double ret = 0.0;
	for (int i=0; i<20; i++) ret+=amps[i]*sin(phase*freqs[i]);
	return ret;
}


static HarmonicsFunction harmonicsFunctions[HARMONICS_END] = {
    [HARMONICS_SINE] = _harmonicFSine,
    [HARMONICS_PIANO] = _harmonicFPiano,
    [HARMONICS_GUITAR] = _harmonicFGuitarNylon,
    [HARMONICS_ELECTRIC_GUITAR] = _harmonicFElectricGuitar,
    [HARMONICS_BASS] = _harmonicFBass,

    [HARMONICS_CHURCH_ORGAN] = _harmonicFChurchOrgan,
    [HARMONICS_CELLO] = _harmonicFCello,
    [HARMONICS_VIOLIN] = _harmonicFViolin,


};






struct voice_timbre getVoiceTimbreForInstrument(enum instruments instrument) {
    if (instrument>=INSTRUMENT_END || instrument<0) return instrumentTimbres[INSTRUMENT_END];
    return instrumentTimbres[instrument];
}





float getVolumeForEnvelope(SynthVoice sv) {
    if (!sv) return 0.0;
    double time=sv->time, offTime=sv->offTime;
    struct voice_timbre timbre = sv->timbre;

    float sustain=timbre.adsr.sustainLevel, release=timbre.adsr.releaseSec, attack=timbre.adsr.attackSec;

    switch (timbre.envelope) {
        case ENVELOPE_EXPONENTIAL: {
            float factor = timbre.adsr.decaySec;
            if (offTime<0) return sustain+(1.0-sustain)*expf(-factor*time);
            else if (offTime>0 && time-offTime<release) {
                float start = sustain+(1.0-sustain)*expf(-factor*offTime);
                return lerpFloat(start, 0.0, (time-offTime)/release);
            } else return 0.0;
        }

        case ENVELOPE_HOLD: {
            if (time<attack) return lerpFloat(0.0, sustain, time/attack);
            else if (offTime<0) return sustain;
            else if (offTime>0 && time-offTime<release) return lerpFloat(sustain, 0.0, (time-offTime)/release);
            return 0.0;
        }

        default:
            return 0.0;
    }
}


float getNormalizedSampleForVoice(SynthVoice sv) {
    if (!sv) return 0.0;
    enum voice_harmonics harmonic = sv->timbre.harmonics;
    if (harmonic<0 || harmonic>=HARMONICS_END) return 0.0;
    if (harmonicsFunctions[harmonic]) return (harmonicsFunctions[harmonic])(sv->phase);
    else return 0.0; 
}