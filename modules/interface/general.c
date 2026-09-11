#include <interface.h>
#include <ui.h>
#include <handler.h>
#include <raymath.h>
#include <math.h>
#include <threads.h>


float interfaceSpace1=0, interfaceSpace2=0;
Vector2 screenSize = {.x=0, .y=0};

int renderInit() {
    updateRenderGlobalVariables(0);

    textFontInit();
    bottomHalfLayoutInit();
    controlLineInit();
    renderTracksLeftInit();
    horizontalKeyboardInit();
    verticalKeyboardInit();
    
    return 0;
}

int renderClose() {
    textFontClose();
    controlLineClose();
    renderTracksLeftClose();
    bottomHalfLayoutClose();
    horizontalKeyboardClose();
    verticalKeyboardClose();
    
    return 0;
}



int updateRenderGlobalVariables(int userInputAllowed) {
    windowToggleFullscreenIfNecessary();

    int newX = GetScreenWidth();
    int newY = GetScreenHeight();

    if (screenSize.y>0) updateBottomHalfSizeOnResize(screenSize, (Vector2){newX, newY});

    screenSize.x = newX;
    screenSize.y = newY;

    updateMouseHandler(userInputAllowed);

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

void renderFPS() {
    int fps = GetFPS();
    Color c1={200,100,120,255}, c2={180,120,120,255}, c3={100,200,120,255};
    renderFontStringAlign(GlobalFonts[0].font, TextFormat("%d", fps), (Vector2){screenSize.x-5, 5}, (Vector2){1, 0}, 30, 0, (fps>55?c3:(fps>40?c2:c1)));
}

int render() {
    int activeBackgroundProcess = threadIsThereActiveBackgroundProcess();
    int allowedInput = (!activeBackgroundProcess && startupInputAllowed());
    
    UIiterationReset();
    if (allowedInput) SetWindowOpacity(1-0.5*GlobalInactivityHandler.opacity);
    
    
    order1Precompute();
    order2Precompute();

    BeginDrawing();
        renderMainBackground();
        renderTracksLeft();
        renderBottomHalfLayout();
        
        
        renderLayoutLines();
        
        renderTracksLeftLayoutsIfAny();

        renderControlLine();

        UIupdateTransparentOverlay();
        renderInstrumentPicker();

        if (globalMouseHandler.pos.y>controlLineHeight+interfaceSpace2 && allowedInput) renderFPS();

        windowRenderBackgroundProcess();
        startupScreenRenderIfNeeded(screenSize);

    EndDrawing();

    if (allowedInput) updateMouseCursor();

    return 0;
}