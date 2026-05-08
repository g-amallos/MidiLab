#include <interface.h>
#include <ui.h>
#include <handler.h>
#include <raymath.h>
#include <math.h>


float interfaceSpace1=0, interfaceSpace2=0;
Vector2 screenSize = {.x=0, .y=0};

int renderInit() {
    textFontInit();
    controlLineInit();
    renderTracksLeftInit();
    return 0;
}

int renderClose() {
    textFontClose();
    controlLineClose();
    renderTracksLeftClose();
    return 0;
}



int updateRenderGlobalVariables() {
    screenSize.x = GetScreenWidth();
    screenSize.y = GetScreenHeight();

    updateMouseHandler();

    return 0;
}

void order1Precompute() {
    order1PrecomputeControlLine();
    order1PrecomputeTracksLeft();
    order1PrecomputeBottomHalfLayout();
}

void order2Precompute() {
    order2PrecomputeControlLine();
    order2PrecomputeTracksLeft();
    order2PrecomputeBottomHalfLayout();
}

int render() {
    
    SetWindowOpacity(1-0.5*GlobalInactivityHandler.opacity);
    UIiterationReset();
    
    order1Precompute();
    order2Precompute();

    BeginDrawing();
        renderMainBackground();
        renderTracksLeft();
        renderBottomHalfLayout();
        renderControlLine();
        renderTracksLeftLayoutsIfAny();

    EndDrawing();
    updateMouseCursor();

    return 0;
}