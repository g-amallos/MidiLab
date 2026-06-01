#include "backend_internal.h"
#include <stdio.h>


int backendInit() {
    int ret = createNewProject();
    globalStateHandlerInit();
    return ret;
}


int backendClose() {
    freeProjectContents(globalProject);
    return 0;
}