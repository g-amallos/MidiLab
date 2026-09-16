#include "backend_internal.h"
#include <visualizer.h>
#include <stdio.h>


int backendInit() {
    int ret = createNewProject();
    globalStateHandlerInit();
    //updateWindowProjectTitle();
    FFTinit();
    return ret;
}


int backendClose() {
    freeProjectContents();
    globalHandlerClearNotesSelected();
    projectClose();
    FFTclose();
    return 0;
}