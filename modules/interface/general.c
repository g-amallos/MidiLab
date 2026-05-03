#include <interface.h>
#include <handler.h>
#include <raymath.h>
#include <math.h>



Vector2 screenSize = {.x=0, .y=0};

int renderInit() {
    textFontInit();
    controlLineInit();
    return 0;
}

int renderClose() {
    textFontClose();
    controlLineClose();
    return 0;
}



int updateRenderGlobalVariables() {
    screenSize.x = GetScreenWidth();
    screenSize.y = GetScreenHeight();

    updateMouseHandler();

    return 0;
}

int render() {
    
    SetWindowOpacity(1-0.5*GlobalInactivityHandler.opacity);


    BeginDrawing();
        renderMainBackground();
        renderControlLine();

        //DrawFPS(10, 10);
    EndDrawing();
    return 0;
}