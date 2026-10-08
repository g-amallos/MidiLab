#include <mlb.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


int projectScale(char** argv) {
    const char* src=argv[0];
    const char* dest=argv[1];
    float scale=atof(argv[2]);
    printf("Arguments passed: `%s`, `%s`, `%f`\n", src, dest, scale);

    if (scale <=0 || scale>10 || importProjectFrom(src)) return 1;

    printf("Project loaded\n");


    uint16_t tnum = globalProject->tracksNum;
    for (uint16_t i=0; i<tnum; i++) {
        Track tr = globalProject->tracks+i;
        uint32_t n=tr->numElements;
        for (uint32_t j=0; j<n; j++) {
            Note nt = tr->notes[j];
            nt->timestamp = (uint32_t)(nt->timestamp*scale);
            nt->duration = (uint32_t)(nt->duration*scale);
        }
    }

    globalProject->tempo = (uint16_t)(scale*globalProject->tempo);

    printf("Project edited\n");


    exportProjectTo(dest);
    freeGlobalProject();
    printf("Project saved\n");

    return 0;
}