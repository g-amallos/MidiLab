#include <backend.h>
#include <stdio.h>


int backendInit() {
    int ret = createNewProject();
    return ret;
}


int backendClose() {
    freeProjectContents(globalProject);
    return 0;
}