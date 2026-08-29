#include <raylib.h>
#include <stdio.h>
#include <interface.h>
#include <handler.h>
#include <backend.h>
#include <synth.h>
#include <images.h>


#define APP_NAME "MidiLab"


AudioStream stream={.buffer=NULL, .channels=2, .sampleRate=44100};
static Vector2 windowSize = {800, 600};


int AppInit() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_ALWAYS_RUN);    // FLAG_WINDOW_UNDECORATED
    
    InitWindow((int)windowSize.x, (int)windowSize.y, APP_NAME);

    SetWindowMinSize(450, 300);
    SetTargetFPS(60);

    startupSetup(windowSize);


    InitAudioDevice();

    backendInit();
    midiInit();
    synthInit();
    iconsInit();
    imagesInit();
    renderInit();

    stream = LoadAudioStream(44100, 32, 2);
    SetAudioStreamCallback(stream, audioInputCallback);
    PlayAudioStream(stream);

    startupScreenRender(screenSize);
    
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    //SetWindowState(FLAG_WINDOW_UNFOCUSED);
    
    return 0;
}


int AppClose() {
    renderClose();
    iconsClose();
    imagesClose();
    backendClose();
    

    StopAudioStream(stream);
    UnloadAudioStream(stream);
    
    synthClose();
    midiClose();
    actionClose();
    midiActionClose();

    CloseAudioDevice();
    CloseWindow();

    return 0;
}

int gameLoop(int (*func)()) {
    while (1) {
        if (WindowShouldClose() && windowShouldCloseDialog()) break;
        func();
    }
    return 0;
}


int testIteration() {
    startupScreenUpdate();
    int inputAllowed = startupInputAllowed();
    int renderAllowed = startupRenderAllowed();

    updateRenderGlobalVariables();      // First update global values

    if (inputAllowed && renderAllowed) {
        globalHandlerUpdateTick();
        updateInactivityStruct();
    }

    if (renderAllowed) render();
    else startupScreenRender(screenSize);

    if (inputAllowed && renderAllowed) {
        actionExecuteAllDeferred();
        midiActionExecuteFrame();
    }

    return 0;
}


int main() {
    AppInit();
    gameLoop(testIteration);
    AppClose();
    return 0;
}