#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define MAX_SAMPLES 30
#define MIN_AMP 0.00001


typedef struct {
    double frequency;
    double db; 
} FreqNode;

typedef struct {
    double freq;
    double amp;
    double db;
} FreqAmp;

typedef struct {
    int num;
    FreqAmp* arr;
} ProcessedFreqs;


double freqLerp(FreqNode f1, FreqNode f2, double freq) {
    if (freq>f2.frequency || freq<f1.frequency) return -1e100;
    return f1.db+(f2.db-f1.db)*(freq-f1.frequency)/(f2.frequency-f1.frequency);
}

double findDBforFreq(double freq, int samples, FreqNode* sampleArr) {
    if (!sampleArr) return -1e100;

    if (freq<sampleArr[0].frequency || freq>sampleArr[samples-1].frequency) {
        return -1e100;
    }

    int low=0, high=samples-2;

    while (low<=high) {
        int mid = low+(high-low)/2;

        if (sampleArr[mid].frequency<=freq && freq<=sampleArr[mid+1].frequency) {
            return freqLerp(sampleArr[mid], sampleArr[mid+1], freq);
        } else if (sampleArr[mid].frequency > freq) {
            high = mid-1;
        } else {
            low = mid+1;
        }
    }
    return -1e100;
}


double getFreqHarmonic(double freq, int harmonic) {
    if (harmonic<0) return freq/harmonic;
    else if (harmonic==0) return 0;
    else return freq*harmonic;
}


ProcessedFreqs processFrequencies(double baseFreq, int samples, FreqNode* sampleArr) {
    ProcessedFreqs ret = {0, NULL};

    
    int harmonics[] = {-4,-3,-2,-1,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
    FreqAmp tmp[MAX_SAMPLES];
    int s = sizeof(harmonics)/sizeof(int);
   


    double sum = 0.0;

    for (int i=0; i<s; i++) {
        double freq = getFreqHarmonic(baseFreq, harmonics[i]);
        double db = findDBforFreq(freq, samples, sampleArr);
        double amp = pow(10.0, db*0.05);
        tmp[i]=(FreqAmp){.freq=freq/baseFreq, .db=db, .amp=amp};
        sum += amp;
    }

    int actualSize = 0;
    for (int i=0; i<s; i++) {
        tmp[i].amp /= sum;
        if (tmp[i].amp>MIN_AMP) actualSize++;
    }
    if (!actualSize) return ret;

    FreqAmp* arr = malloc(sizeof(FreqAmp)*actualSize);
    if (!arr) return ret;
    ret.arr = arr;
    ret.num = actualSize;

    int index=0;
    for (int i=0; i<s; i++) {
        if (tmp[i].amp>MIN_AMP) arr[index++]=tmp[i];
    }

    
    return ret;
}


void writeToFile(ProcessedFreqs pf) {
    if (!(pf.arr)) return;

    FILE* fptr = fopen("new.c", "w");
    if (!fptr) {
        fprintf(stderr, "Couldn't open `new.c`\n");
        return;
    }

    const char* s1 = "// Generated automatically\nstatic float waveSampleInstrument(double phase) {\n\tdouble freqs";
    const char* s2 = "\n\tdouble ret = 0.0;\n\tfor (int i=0; i<";
    const char* s3 = "; i++) ret+=amps[i]*sin(phase*freqs[i]);\n\treturn ret;\n}";

    fprintf(fptr, "%s[%d] = {", s1, pf.num);
    for (int i=0; i<pf.num; i++) if (i<pf.num-1) fprintf(fptr, "%g, ", pf.arr[i].freq); else fprintf(fptr, "%g};\n\tdouble amps[%d] = {", pf.arr[i].freq, pf.num);
    for (int i=0; i<pf.num; i++) if (i<pf.num-1) fprintf(fptr, "%g, ", pf.arr[i].amp); else fprintf(fptr, "%g};", pf.arr[i].amp);
    fprintf(fptr, "%s%d%s", s2, pf.num, s3);
    fclose(fptr);
}


int main(int argc, char** argv) {
    if (argc<2) {
        fprintf(stderr, "Specify base frequency\n");
        return 1;
    }

    double baseFreq = atof(argv[1]);
    if (baseFreq<20) {
        fprintf(stderr, "Invalid frequency\n");
        return 1;
    }

    FILE* fptr = fopen("spectrum.txt", "r");
    if (!fptr) {
        fprintf(stderr, "Couldn't open `spectrum.txt`\n");
        return 1;
    }

    int lines=0, c;
    while (1) {
        c=fgetc(fptr);
        if (c==EOF) break;
        if (c=='\n') lines++;
    }

    int samples = lines-1;

    fseek(fptr, 0, SEEK_SET);

    char buff[1024] = "Frequency (Hz)	Level (dB)";
    fgets(buff, 1024, fptr);

    FreqNode* freqArr = malloc(sizeof(FreqNode)*samples);
    if (!freqArr) {
        fprintf(stderr, "Couldn't allocate the memory needed for %d FreqNodes\n", samples);
        fclose(fptr);
        return 1;
    }


    int brk = 0;
    for (int i=0; i<samples; i++) {
        if (fscanf(fptr, "%lf", &(freqArr[i].frequency))!=1) {
            brk=1;
            break;
        }
        if (fscanf(fptr, "%lf", &(freqArr[i].db))!=1) {
            brk=1;
            break;
        }
        //printf("Detected this pair: freq=%6.2lf, db=%6.2lf\n", freqArr[i].frequency, freqArr[i].db);
    }

    if (brk) {
        fprintf(stderr, "Couldn't parse all of the lines properly\n");
        fclose(fptr);
        free(freqArr);
        return 1;
    }


    ProcessedFreqs data = processFrequencies(baseFreq, samples, freqArr);
    free(freqArr);
    fclose(fptr);
    if (!(data.arr)) {
        fprintf(stderr, "`processFrequencies` returned a NULL array\n");
        return 1;
    }


    writeToFile(data);
    free(data.arr);

    return 0;
}