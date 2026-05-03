#include <raylib.h>
#include <stdint.h>
#include <images.h>


struct texture {
    const char* path;
    Texture2D texture;
};



struct texture icons[T_ICON_END] = {
    [T_ICON_MENU] = {.path="assets/icons/menu.png", .texture={0}},
    [T_ICON_RIGHT] = {.path="assets/icons/right.png", .texture={0}},
    [T_ICON_SETTINGS] = {.path="assets/icons/settings.png", .texture={0}},
    [T_ICON_EDIT] = {.path="assets/icons/edit.png", .texture={0}},

};

void _loadTexture(struct texture* txtr) {
    if (txtr->path && !(txtr->texture.id)) txtr->texture = LoadTexture(txtr->path);
}

void _unloadTexture(struct texture* txtr) {
    if (txtr->path && txtr->texture.id) {
        UnloadTexture(txtr->texture);
        txtr->texture.id = 0;
    }
}

int iconsInit() {
    for (int i=0; i<T_ICON_END; i++) {
        _loadTexture(icons+i);
    }
    return 0;
}

int iconsClose() {
    for (int i=0; i<T_ICON_END; i++) {
        _unloadTexture(icons+i);
    }
    return 0;
}

Texture2D iconGetTexture(enum icon_title icon) {
    if (icon>=T_ICON_END || icon<0) return (Texture2D){0};
    return icons[icon].texture;
}

Vector2 iconGetDimensions(enum icon_title icon) {
    if (icon>=T_ICON_END || icon<0) return (Vector2){0, 0};
    Texture2D txtr = icons[icon].texture;
    return (Vector2){txtr.width, txtr.height};
}

void iconRerder(enum icon_title icon, Rectangle rect, Color col) {
    if (icon>=T_ICON_END || icon<0) return;
    Texture2D txtr = icons[icon].texture;
    DrawTexturePro(txtr, (Rectangle){0, 0, txtr.width, txtr.height}, rect, (Vector2){0, 0}, 0, col);
}