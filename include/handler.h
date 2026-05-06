#ifndef HANDLER_H
#define HANDLER_H

#include <stdint.h>


/* Inactivity Handler (handler/inactivity.c) */

struct inactivityHandler {
    double currentT;
    double lastActiveT;
    float inactiveForT;
    float opacity;
};

extern struct inactivityHandler GlobalInactivityHandler;
void updateInactivityStruct();



struct mouse_handler {
    Vector2 pos;
    Vector2 dpos;
    uint8_t pressed;
    uint8_t released;
    uint8_t down;
    uint8_t rightClickPressed;
    float scroll;
};

extern struct mouse_handler globalMouseHandler;
void setNextMouseCursor(int cursor);
void updateMouseCursor();
void updateMouseHandler();



#endif