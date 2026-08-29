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
    [T_ICON_MIDI] = {.path="assets/icons/midi.png", .texture={0}},
    [T_ICON_LOOP] = {.path="assets/icons/loop.png", .texture={0}},
    [T_ICON_NEXT] = {.path="assets/icons/next.png", .texture={0}},
    [T_ICON_PREVIOUS] = {.path="assets/icons/previous.png", .texture={0}},
    [T_ICON_PLAY] = {.path="assets/icons/play.png", .texture={0}},
    [T_ICON_PAUSE] = {.path="assets/icons/pause.png", .texture={0}},
    [T_ICON_UNDO] = {.path="assets/icons/undo.png", .texture={0}},
    [T_ICON_REDO] = {.path="assets/icons/redo.png", .texture={0}},
    [T_ICON_PIANO] = {.path="assets/icons/piano.png", .texture={0}},
    [T_ICON_KEYBOARD] = {.path="assets/icons/keyboard.png", .texture={0}},
    [T_ICON_VOLUME_NONE] = {.path="assets/icons/volume-none.png", .texture={0}},
    [T_ICON_VOLUME_MIN] = {.path="assets/icons/volume-min.png", .texture={0}},
    [T_ICON_VOLUME_MID] = {.path="assets/icons/volume-mid.png", .texture={0}},
    [T_ICON_VOLUME_MAX] = {.path="assets/icons/volume-max.png", .texture={0}},
    [T_ICON_SAVE] = {.path="assets/icons/save.png", .texture={0}},
    [T_ICON_FOLDER] = {.path="assets/icons/folder.png", .texture={0}},
    [T_ICON_ADD] = {.path="assets/icons/add.png", .texture={0}},
    [T_ICON_DELETE] = {.path="assets/icons/delete.png", .texture={0}},
    [T_ICON_COPY] = {.path="assets/icons/copy.png", .texture={0}},
    [T_ICON_PASTE] = {.path="assets/icons/paste.png", .texture={0}},

    [T_ICON_OPTIONS] = {.path="assets/icons/options.png", .texture={0}},
    [T_ICON_BASS] = {.path="assets/icons/bass.png", .texture={0}},
    [T_ICON_BRASS] = {.path="assets/icons/brass.png", .texture={0}},
    [T_ICON_CONTRABASS] = {.path="assets/icons/contrabass.png", .texture={0}},
    [T_ICON_DRUMS] = {.path="assets/icons/drums.png", .texture={0}},
    [T_ICON_ELECTRIC_GUITAR] = {.path="assets/icons/electric-guitar.png", .texture={0}},
    [T_ICON_GUITAR] = {.path="assets/icons/guitar.png", .texture={0}},
    [T_ICON_MICROPHONE] = {.path="assets/icons/microphone.png", .texture={0}},
    [T_ICON_ORGAN] = {.path="assets/icons/organ.png", .texture={0}},
    [T_ICON_PAD] = {.path="assets/icons/pad.png", .texture={0}},
    [T_ICON_PERCUSSION] = {.path="assets/icons/percussion.png", .texture={0}},
    [T_ICON_VIOLIN] = {.path="assets/icons/violin.png", .texture={0}},
    [T_ICON_FLUTE] = {.path="assets/icons/flute.png", .texture={0}},
    [T_ICON_EFFECTS] = {.path="assets/icons/effects.png", .texture={0}},

    [T_ICON_VIEW_NONE] = {.path="assets/icons/none.png", .texture={0}},
    [T_ICON_VIEW_ROLL] = {.path="assets/icons/notes.png", .texture={0}},

    [T_ICON_MIDILAB_LOGO_256] = {.path="assets/images/midilablogo/midilab_256.png", .texture={0}},
    [T_ICON_MIDILAB_LOGO_1024] = {.path="assets/images/midilablogo/midilab_1024.png", .texture={0}},

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

int iconsPreInit() {
    _loadTexture(icons+T_ICON_MIDILAB_LOGO_1024);
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