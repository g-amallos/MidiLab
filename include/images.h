#ifndef IMAGES_H
#define IMAGES_H

#include <raylib.h>

enum icon_title {
    T_ICON_MENU,
    T_ICON_RIGHT,
    T_ICON_SETTINGS,
    T_ICON_EDIT,
    T_ICON_MIDI,
    T_ICON_UNDO,
    T_ICON_REDO,
    T_ICON_PLAY,
    T_ICON_PAUSE,
    T_ICON_PREVIOUS,
    T_ICON_NEXT,
    T_ICON_LOOP,
    T_ICON_PIANO,
    T_ICON_KEYBOARD,

    T_ICON_END
};




int iconsInit();
int iconsClose();
Texture2D iconGetTexture(enum icon_title icon);
Vector2 iconGetDimensions(enum icon_title icon);
void iconRerder(enum icon_title icon, Rectangle rect, Color col);



#endif