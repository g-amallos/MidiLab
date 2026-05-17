#include <interface.h>
#include <backend.h>
#include <raymath.h>
#include <colors.h>
#include <utils.h>
#include <ui.h>
#include <stdlib.h>
#include <stdio.h>
#include <images.h>
#include <handler.h>








void renderLayoutLines() {
    float botHalfY = screenSize.y-bottomHalfHeight;
    Color lineCol = {100, 102, 108, 255};
    DrawLineEx((Vector2){trackLeftWidth, controlLineHeight}, (Vector2){trackLeftWidth, botHalfY}, 2, lineCol);
    DrawLineEx((Vector2){0,botHalfY}, (Vector2){screenSize.x,botHalfY}, 2, lineCol);
}