#include <raylib.h>
#include <handler.h>






struct mouse_handler globalMouseHandler = {
    .pos = {0, 0},
    .dpos = {0, 0},
    .clickPos ={0, 0},
    .down = 0,
    .pressed = 0,
    .released = 0,
    .rightClickPressed = 0,
    .scroll = 0,
};


int nextMouseCursor = MOUSE_CURSOR_ARROW;

void setNextMouseCursor(int cursor) {
    nextMouseCursor = cursor;
}

void updateMouseCursor() {
    SetMouseCursor(nextMouseCursor);
}

void updateMouseHandler() {
    setNextMouseCursor(MOUSE_CURSOR_ARROW);
    globalMouseHandler.pos = GetMousePosition();
    globalMouseHandler.dpos = GetMouseDelta();
    globalMouseHandler.pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (globalMouseHandler.pressed) globalMouseHandler.clickPos = globalMouseHandler.pos;
    globalMouseHandler.rightClickPressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    globalMouseHandler.down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    globalMouseHandler.released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    globalMouseHandler.scroll = GetMouseWheelMoveV().y;
}