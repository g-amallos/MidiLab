#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define MAX_SAMPLES 20
#define MIN_AMP 0.0002


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


int comp(const void *a, const void *b) {
    FreqNode* sa = (FreqNode*)a;
    FreqNode* sb = (FreqNode*)b;
    return (sb->db - sa->db);
}

double absolute(double v) {
    if (v<0) return -v;
    return v;
}

double quantize(double rf) {
    /*
    double lg = round(48*log2(rf));
    return exp2(lg/48.0);
    */
   double lg = 12*log2(rf);
   if (absolute(lg-round(lg))<0.2) return exp2(round(lg)/12.0);
   return rf;
}


ProcessedFreqs processFrequencies(double baseFreq, int samples, FreqNode* sampleArr) {
    ProcessedFreqs ret = {0, NULL};

    FreqAmp* arr = malloc(sizeof(FreqAmp)*MAX_SAMPLES);
    if (!arr) {
        fprintf(stderr, "In `processFrequencies` malloc returned NULL\n");
        return ret;
    }

    printf("arr=%p\n", arr);

    qsort(sampleArr, samples, sizeof(FreqNode), comp);

    double totalDb = -1e100, sum=0, temp, qbf=quantize(baseFreq);
    int actualNum=0;
    for (int i=0; i<MAX_SAMPLES; i++) {
        arr[i].freq = quantize(sampleArr[i].frequency)/qbf;

        arr[i].db = sampleArr[i].db;
        arr[i].amp = pow(10.0, arr[i].db/20.0);
        if (arr[i].amp < MIN_AMP) {
            break;
        }
        actualNum++;
        sum+=arr[i].amp;
    }

    if (actualNum<MAX_SAMPLES) arr = realloc(arr, sizeof(FreqAmp)*actualNum);   // How could this possibly fail

    for (int i=0; i<actualNum; i++) {
        arr[i].amp /= sum; 
    }

    double* diff = malloc(sizeof(double)*actualNum);
    if (!diff) {
        fprintf(stderr, "In `processFrequencies` malloc returned NULL -2\n");
        free(arr);
        return ret;
    }
    for (int i=0; i<actualNum; i++) diff[i] = -1;

    for (int i=0; i<actualNum; i++) {
        for (int j=0; j<=i; j++) {
            if (diff[j]==arr[i].freq) break;
            if (diff[j]==-1) {
                diff[j] = arr[i].freq;
                break;
            }
        }
    }
    int n=0;
    while (n<actualNum && diff[n]!=-1) n++;

    FreqAmp* dt = malloc(sizeof(FreqAmp)*n);
    if (!dt) {
        fprintf(stderr, "In `processFrequencies` malloc returned NULL -3\n");
        free(arr);
        free(diff);
        return ret;
    }

    for (int i=0; i<n; i++) {
        dt[i].freq = diff[i];
        dt[i].db = -1e100;
        dt[i].amp = 0;
        for (int j=0; j<actualNum; j++) {
            if (arr[j].freq == diff[i]) dt[i].amp += arr[j].amp;
        }
    }

    free(arr);
    free(diff);

    ret.num = n;
    ret.arr = dt;
    
    
    printf("Finished processing with arr=%p, num=%d\n", arr, actualNum);
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