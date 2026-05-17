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




float bottomHalfHeight=0, bottomHalfUsefulHeight=0, resizeButtonHitboxSize=10, bottomHalfLayoutTopPadding=10;
Button bottomResizeButton = NULL;


void updateBottomHalfSize(float newSize) {
    enum keyboard_render_types rtype = globalStateHandlerGetKeyboardType();
    float maxH = (rtype==T_KEYBOARD_HORIZONTAL)?horizontalKeyboardHeight:((rtype==T_KEYBOARD_NONE)?0:(0.8*screenSize.y-controlLineHeight));
    bottomHalfHeight = floatClip(newSize, floatMin(resizeButtonHitboxSize, maxH), maxH);
    bottomHalfUsefulHeight = bottomHalfHeight-bottomHalfLayoutTopPadding;
    //printf("Tried: %f | Got: %f\n", newSize, bottomHalfHeight);
}


void bottomHalfLayoutInit() {
    bottomResizeButton = buttonCreate((Rectangle){20,20,20,20}, 0);
    buttonUpdateCursorOnHover(bottomResizeButton, MOUSE_CURSOR_RESIZE_NS);

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

    enum keyboard_render_types kbType = globalStateHandlerGetKeyboardType();
    if (kbType==T_KEYBOARD_HORIZONTAL) precalculateJustHorizontalKeyboard();
}


void renderBottomHalfLayout() {
    enum keyboard_render_types kbType = globalStateHandlerGetKeyboardType();
    if (kbType==T_KEYBOARD_NONE) return;

    DrawRectangleRec((Rectangle){0, screenSize.y-bottomHalfHeight, screenSize.x, bottomHalfHeight}, (Color){20, 20, 24, 255});
    DrawRectangleRounded(scaleRctangleFromCenterV(buttonGetRectangle(bottomResizeButton), (Vector2){0.08, 0.3}), 1, 5, (Color){120, 125, 132, 255});
    if (kbType==T_KEYBOARD_HORIZONTAL) renderHorizontalKeyboard();
}