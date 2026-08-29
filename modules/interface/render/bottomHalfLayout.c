#include <interface.h>
#include <backend.h>
#include <handler.h>
#include <raymath.h>
#include <colors.h>
#include <utils.h>
#include <ui.h>
#include <stdlib.h>
#include <stdio.h>
#include <images.h>
#include <synth.h>



float bottomHalfHeight=0, bottomHalfUsefulHeight=0, resizeButtonHitboxSize=10, bottomHalfLayoutTopPadding=10;
Button bottomResizeButton = NULL;
Color bottomHalfBackgroundColor={0,0,0,0};


void updateBottomHalfSize(float newSize) {
    enum keyboard_render_types rtype = globalStateHandlerGetKeyboardType();
    float maxH = (rtype==T_KEYBOARD_HORIZONTAL)?(horizontalKeyboardHeight+bottomHalfLayoutTopPadding):((rtype==T_KEYBOARD_NONE)?0:(0.8*screenSize.y-controlLineHeight+bottomHalfLayoutTopPadding));
    bottomHalfHeight = floatClip(newSize, floatMin(resizeButtonHitboxSize, maxH), maxH);
    bottomHalfUsefulHeight = bottomHalfHeight-bottomHalfLayoutTopPadding;
    //printf("Tried: %f | Got: %f\n", newSize, bottomHalfHeight);
}


void bottomHalfLayoutInit() {
    bottomResizeButton = buttonCreate((Rectangle){20,20,20,20}, 0);
    buttonUpdateCursorOnHover(bottomResizeButton, MOUSE_CURSOR_RESIZE_NS);

    bottomHalfBackgroundColor=(Color){20, 20, 24, 255};

    updateBottomHalfSize(0);
}

void bottomHalfLayoutClose() {
    if (bottomResizeButton) buttonFree(bottomResizeButton);
    bottomResizeButton=NULL;
}





void order1PrecomputeBottomHalfLayout() {
    static enum keyboard_render_types oldType=T_KEYBOARD_NONE;

    enum keyboard_render_types kbType = globalStateHandlerGetKeyboardType();
    if (kbType==T_KEYBOARD_NONE) buttonDisable(bottomResizeButton);
    else buttonEnable(bottomResizeButton);


    if (oldType!=kbType) {
        if (oldType==T_KEYBOARD_NONE) updateBottomHalfSize(0.7*screenSize.y);
        else if (kbType==T_KEYBOARD_NONE) updateBottomHalfSize(0);
        //printf("Old: %d | New: %d | Update: %d | NewVal: %f\n", oldType, kbType, oldType!=kbType, bottomHalfHeight);
        oldType=kbType;
    }

    Rectangle rect = {0, screenSize.y-bottomHalfHeight+1, screenSize.x, resizeButtonHitboxSize};
    buttonUpdateRectangle(bottomResizeButton, rect);
    buttonUpdate(bottomResizeButton, -1);
    if (isButtonDragged(bottomResizeButton)) {
        float maxf=screenSize.y-resizeButtonHitboxSize, minf=controlLineHeight+0.2*screenSize.y;
        float val=floatClip(globalMouseHandler.pos.y-0.5*resizeButtonHitboxSize, minf, maxf);
        bottomHalfHeight = screenSize.y-val;
    }

    updateBottomHalfSize(bottomHalfHeight);
}

void updateBottomHalfSizeOnResize(Vector2 oldScreen, Vector2 newScreen) {
    //enum keyboard_render_types rtype = globalStateHandlerGetKeyboardType();
    float ratio = bottomHalfHeight/oldScreen.y;
    updateBottomHalfSize(ratio*newScreen.y);
}


void order2PrecomputeBottomHalfLayout() {
    Rectangle rect = {0, screenSize.y-bottomHalfHeight+1, screenSize.x, resizeButtonHitboxSize};
    buttonUpdateRectangle(bottomResizeButton, rect);

    precalculateSizesHorizontalKeyboard();
    preCalculateNecessaryVerticalKeyboard();

    enum keyboard_render_types kbType = globalStateHandlerGetKeyboardType();
    if (kbType==T_KEYBOARD_HORIZONTAL) {
        bottomHalfBackgroundColor=(Color){20, 20, 24, 255};
        precalculateJustHorizontalKeyboard();
    } else if (kbType==T_KEYBOARD_VERTICAL) {
        bottomHalfBackgroundColor=(Color){20, 20, 24, 255};
        preCalculateVerticalKeyboard();
        order2PrecomputeRoll();
    }


    int allowedKeyboardShortcuts = (!UIisInTextInput() && !UIexistsFrontLayoutOverlay());
    if (allowedKeyboardShortcuts) {
        if (IsKeyPressed(KEY_M)) synthPanic();
    }
}


void renderBottomHalfLayout() {
    enum keyboard_render_types kbType = globalStateHandlerGetKeyboardType();
    if (kbType==T_KEYBOARD_NONE) return;


    float ypos = screenSize.y-bottomHalfHeight-interfaceSpace1;
    DrawRectangleGradientV(0, ypos, screenSize.x, interfaceSpace1, (Color){5,5,5,0}, (Color){5,5,5,160});

    DrawRectangleRec((Rectangle){0, screenSize.y-bottomHalfHeight, screenSize.x, bottomHalfHeight}, bottomHalfBackgroundColor);
    
    if (kbType==T_KEYBOARD_HORIZONTAL) renderHorizontalKeyboard();
    else if (kbType==T_KEYBOARD_VERTICAL) {
        renderWholeBottomLayoutTypeVertical();
    }

    DrawRectangleRec((Rectangle){0, screenSize.y-bottomHalfHeight, screenSize.x, bottomHalfLayoutTopPadding}, bottomHalfBackgroundColor);
    DrawRectangleRounded(scaleRctangleFromCenterV(buttonGetRectangle(bottomResizeButton), (Vector2){0.08, 0.3}), 1, 5, (Color){120, 125, 132, 255});
}