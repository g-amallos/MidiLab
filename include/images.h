#ifndef IMAGES_H
#define IMAGES_H

#include <raylib.h>

enum icon_title {
    T_ICON_MENU,
    T_ICON_RIGHT,
    T_ICON_SETTINGS,
    T_ICON_EDIT,

    T_ICON_END
};




int iconsInit();
int iconsClose();
Texture2D iconGetTexture(enum icon_title icon);
Vector2 iconGetDimensions(enum icon_title icon);
void iconRerder(enum icon_title icon, Rectangle rect, Color col);



#endif