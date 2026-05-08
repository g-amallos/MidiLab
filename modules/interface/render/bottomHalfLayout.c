#include <interface.h>
#include <backend.h>
#include <raymath.h>
#include <colors.h>
#include <utils.h>
#include <ui.h>
#include <stdlib.h>
#include <stdio.h>
#include <images.h>




float bottomHalfHeight=0;


void order1PrecomputeBottomHalfLayout() {
    bottomHalfHeight = 0; //0.5*screenSize.y;
}

void order2PrecomputeBottomHalfLayout() {
    
}


void renderBottomHalfLayout() {
    DrawRectangleRec((Rectangle){0, screenSize.y-bottomHalfHeight, screenSize.x, bottomHalfHeight}, (Color){35, 35, 40, 255});
}