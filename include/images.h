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

    T_ICON_VOLUME_NONE,
    T_ICON_VOLUME_MIN,
    T_ICON_VOLUME_MID,
    T_ICON_VOLUME_MAX,
    T_ICON_ADD,
    T_ICON_SAVE,
    T_ICON_FOLDER,

    T_ICON_OPTIONS,
    T_ICON_BASS,
    T_ICON_BRASS,
    T_ICON_CONTRABASS,
    T_ICON_DRUMS,
    T_ICON_ELECTRIC_GUITAR,
    T_ICON_GUITAR,
    T_ICON_MICROPHONE,
    T_ICON_ORGAN,
    T_ICON_PAD,
    T_ICON_PERCUSSION,
    T_ICON_VIOLIN,

    T_ICON_END
};




int iconsInit();
int iconsClose();
Texture2D iconGetTexture(enum icon_title icon);
Vector2 iconGetDimensions(enum icon_title icon);
void iconRerder(enum icon_title icon, Rectangle rect, Color col);



#endif