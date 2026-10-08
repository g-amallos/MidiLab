#include <editor.h>
#include <stdio.h>
#include <string.h>


struct command {
    const char* name;
    int num;
    int (*func)(char**);
};



int main(int argc, char** argv) {
    struct command commands[] = {{"scale", 3, projectScale}};
    int s=sizeof(commands)/sizeof(struct command);
    int found=0, exitCode=0;

    if (argc>1) {
        for (int i=0; i<s; i++) {
            if (!strcmp(commands[i].name, argv[1])) {
                if (argc>commands[i].num+1) {
                    found=1;
                    exitCode = commands[i].func(argv+2);
                } else {
                    fprintf(stderr, "Command `%s` expects %d arguments\n", argv[1], commands[i].num);
                    return 1;
                }
            }
        }
    }

    if (!found) {
        fprintf(stderr, "Usage with commands\n");
        exitCode = 1;
    }

    return exitCode;
}