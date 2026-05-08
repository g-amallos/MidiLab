#include <raylib.h>
#include <stdio.h>
#include <interface.h>
#include <handler.h>
#include <backend.h>
#include <images.h>


#define APP_NAME "MidiLab"


int AppInit() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_ALWAYS_RUN);    // FLAG_WINDOW_UNDECORATED
    InitWindow(1200, 800, APP_NAME);
    SetWindowMinSize(450, 300);
    SetTargetFPS(60);

    backendInit();
    iconsInit();
    renderInit();
    
    return 0;
}


int AppClose() {
    renderClose();
    iconsClose();
    backendClose();
    CloseWindow();
    return 0;
}

int gameLoop(int (*func)()) {
    while (!WindowShouldClose()) {
        func();
    }
    return 0;
}


int testIteration() {
    updateRenderGlobalVariables();      // First update global values
    updateInactivityStruct();

    render();

    actionExecuteAllDeferred();
    return 0;
}


int main() {
    AppInit();
    gameLoop(testIteration);
    AppClose();
    return 0;
}