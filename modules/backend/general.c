#include "backend_internal.h"
#include <stdio.h>


int backendInit() {
    int ret = createNewProject();
    globalStateHandlerInit();
    updateWindowProjectTitle();
    return ret;
}


int backendClose() {
    freeProjectContents(globalProject);
    projectClose();
    return 0;
}