#include <interface.h>
#include <ui.h>
#include <handler.h>
#include <raymath.h>
#include <math.h>


float interfaceSpace1=0, interfaceSpace2=0;
Vector2 screenSize = {.x=0, .y=0};

int renderInit() {
    updateRenderGlobalVariables();

    textFontInit();
    bottomHalfLayoutInit();
    controlLineInit();
    renderTracksLeftInit();
    horizontalKeyboardInit();
    
    return 0;
}

int renderClose() {
    textFontClose();
    controlLineClose();
    renderTracksLeftClose();
    bottomHalfLayoutClose();
    horizontalKeyboardClose();
    return 0;
}



int updateRenderGlobalVariables() {
    int newX = GetScreenWidth();
    int newY = GetScreenHeight();

    if (screenSize.y>0) updateBottomHalfSizeOnResize(screenSize, (Vector2){newX, newY});

    screenSize.x = newX;
    screenSize.y = newY;

    updateMouseHandler();

    return 0;
}

void order1Precompute() {
    precalculateInstrumentPicker();
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
        
        renderLayoutLines();
        
        renderTracksLeftLayoutsIfAny();

        UIupdateTransparentOverlay();
        renderInstrumentPicker();

    EndDrawing();
    updateMouseCursor();

    return 0;
}