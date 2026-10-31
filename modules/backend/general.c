#include "backend_internal.h"
#include <visualizer.h>
#include <stdio.h>


int backendInit() {
    int ret = createNewProject();
    globalStateHandlerInit();
    keyboardInputInit();
    //updateWindowProjectTitle();
    FFTinit();
    return ret;
}


int backendClose() {
    freeProjectContents();
    globalHandlerClearNotesSelected();
    globalStateHandlerClose();
    projectClose();
    FFTclose();
    return 0;
}