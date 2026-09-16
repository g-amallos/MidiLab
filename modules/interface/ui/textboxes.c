#include <stdlib.h>
#include "ui_internal.h"
#include <interface.h>
#include <handler.h>
#include <string.h>
#include <stdio.h>




struct key_press_state {
    float timer;
    float delay;
    float interval;
};

typedef struct ui_textbox {
    Rectangle rect;
    struct ui_element_interaction_values state;
    uint8_t effectSpeed;
    uint8_t justUnfocused;
    uint8_t canBeShadowedByLayout;
    uint8_t disabledByFrontLayout;
    
    char* text;
    int cursorIdx;
    int lastCharIdx;
    int textSize;
    int maxInputChars;

    enum textbox_input_type inputType;

    struct key_press_state backspace;
    struct key_press_state delete;
    struct key_press_state left;
    struct key_press_state right;

    int cursorOnHover;
    float roundness;
    float effect;
} *Textbox;


void updateTextboxEffect(Textbox tbx) {
    tbx->effect += (tbx->state.effectTarget-tbx->effect)/tbx->effectSpeed;
    if (!(tbx->state.disabled) && (tbx->state.hovered || tbx->state.dragging)) setNextMouseCursor(tbx->cursorOnHover);
}

void textboxUpdate(Textbox tbx, int effectTarget) {
    if (!tbx) return;
    tbx->justUnfocused = 0;
    if (tbx->state.disabled) {
        tbx->state.effectTarget = 0;
        tbx->state.hovered = 0;
        tbx->state.pressed = 0;
        tbx->state.dragging = 0;

        tbx->cursorIdx = -1;
        updateTextboxEffect(tbx);
        return;
    }
    tbx->state.hovered = (!(tbx->state.disableHover)) && (!(tbx->disabledByFrontLayout && UIexistsFrontLayoutOverlay())) && (!(tbx->canBeShadowedByLayout && UIisHoveringOverLayout()) && checkCollisionPointRoundedRect(globalMouseHandler.pos, tbx->rect, tbx->roundness));
    
    if (globalMouseHandler.pressed) {
        if (tbx->state.hovered) {
            tbx->state.pressed = 1;
            if (!(tbx->state.focused)) {
                tbx->state.focused = 1;
                tbx->cursorIdx = tbx->lastCharIdx;
            }
            tbx->state.dragging = 1;
        } else {
            tbx->state.pressed = 0;
            tbx->state.dragging = 0;
            if (tbx->state.focused) tbx->justUnfocused = 1;
            tbx->state.focused = 0;
        }
    } else tbx->state.pressed = 0;
    
    if (globalMouseHandler.released || !globalMouseHandler.down) {
        tbx->state.dragging = 0;
    }

    if (effectTarget==1) tbx->state.effectTarget=1;
    else if (effectTarget==0) tbx->state.effectTarget=0;
    else tbx->state.effectTarget = (!(tbx->state.disabled) && (((tbx->state.hovered || tbx->state.focused) && (!globalMouseHandler.down || globalMouseHandler.pressed)) || tbx->state.dragging || tbx->state.pressed));

    if (!(tbx->state.focused)) tbx->cursorIdx = -1;
    if (tbx->state.focused) UIinTextInput();

    updateTextboxEffect(tbx);

}

void textboxDisable(Textbox tbx) {
    if (!tbx) return;
    tbx->state.disabled = 1;
    tbx->state.hovered = 0;
    tbx->state.dragging = 0;
    tbx->state.pressed = 0;
    tbx->state.focused = 0;
    tbx->state.effectTarget = 0;
    tbx->cursorIdx = -1;
}

void textboxEnable(Textbox tbx) {
    if (!tbx) return;
    tbx->state.disabled = 0;
}


void textboxEnableLayoutShadowing(Textbox tbx) {
    if (!tbx) return;
    tbx->canBeShadowedByLayout = 1;
}

void textboxDisableLayoutShadowing(Textbox tbx) {
    if (!tbx) return;
    tbx->canBeShadowedByLayout = 0;
}


void _resetKeyPressState(struct key_press_state* kps) {
    kps->timer = 0;
    kps->delay = 0.5;
    kps->interval = 0.05;
}

Textbox textboxCreate(Rectangle rect, float roundness, enum textbox_input_type inputType, int maxInputChars) {
    Textbox tbx = malloc(sizeof(struct ui_textbox));
    if (!tbx) return NULL;

    char* txt = calloc(32, sizeof(char));
    if (!txt) {
        free(tbx);
        return NULL;
    }

    tbx->text = txt;
    tbx->textSize = 32;
    tbx->lastCharIdx = 0;
    tbx->cursorIdx = -1;
    tbx->maxInputChars = maxInputChars;

    tbx->inputType = inputType;
    tbx->justUnfocused = 0;

    _resetKeyPressState(&(tbx->backspace));
    _resetKeyPressState(&(tbx->delete));
    _resetKeyPressState(&(tbx->left));
    _resetKeyPressState(&(tbx->right));

    tbx->canBeShadowedByLayout = 1;
    tbx->disabledByFrontLayout = 1;
    tbx->rect = rect;
    
    tbx->state.disabled = 0;
    tbx->state.hovered = 0;
    tbx->state.dragging = 0;
    tbx->state.pressed = 0;
    tbx->state.effectTarget = 0;
    tbx->state.focused = 0;
    tbx->state.disableHover = 0;
    
    tbx->cursorOnHover = MOUSE_CURSOR_IBEAM;
    tbx->roundness = roundness;
    tbx->effectSpeed = 4;
    tbx->effect = 0;

    return tbx;
}


void textboxDisableHover(Textbox tbx) {
    if (!tbx) return;
    tbx->state.disableHover = 1;
}

void textboxEnableHover(Textbox tbx) {
    if (!tbx) return;
    tbx->state.disableHover = 0;
}

void textboxFree(Textbox tbx) {
    if (!tbx) return;
    free(tbx->text);
    free(tbx);
}

void textboxEnableOnFrontLayout(Textbox tbx) {
    if (!tbx) return;
    tbx->disabledByFrontLayout = 0;
}

void textboxDisableOnFrontLayout(Textbox tbx) {
    if (!tbx) return;
    tbx->disabledByFrontLayout = 1;
}

void _halveTextBuffer(Textbox tbx) {
    int newSize = (tbx->textSize)>>1;
    char* rel = realloc(tbx->text, newSize);
    if (!rel) return;
    tbx->text = rel;
    tbx->textSize = newSize;
    int applyZero = 0;
    for (int i=0; i<newSize; i++) {
        if (!((tbx->text)[i]) || i==newSize-1) applyZero = 1;
        if (applyZero) (tbx->text)[i] = 0;
    }
}

void _doubleTextBuffer(Textbox tbx) {
    int newSize = (tbx->textSize)<<1;
    char* rel = realloc(tbx->text, newSize);
    if (!rel) return;   // Failed

    tbx->text = rel;
    for (int i=tbx->textSize; i<newSize; i++) (tbx->text)[i] = 0;
    
    tbx->textSize = newSize;
}


void _appendChar(Textbox tbx, char chr) {
    if (tbx->lastCharIdx>=tbx->textSize-1) _doubleTextBuffer(tbx);
    (tbx->text)[(tbx->lastCharIdx)++] = chr;
    (tbx->text)[tbx->lastCharIdx] = 0;
}

void _removeLastChar(Textbox tbx) {
    if (tbx->lastCharIdx) {
        (tbx->text)[--(tbx->lastCharIdx)] = 0;
        if (4*tbx->lastCharIdx < tbx->textSize) _halveTextBuffer(tbx);
    }
}

void _removeCharAtIdx(Textbox tbx, int idx) {
    int valid=0;
    for (int i=idx+1; i<=tbx->lastCharIdx; valid=i++) tbx->text[i-1]=tbx->text[i];
    if (valid) tbx->text[(tbx->lastCharIdx)--]=0;
    if (4*tbx->lastCharIdx < tbx->textSize) _halveTextBuffer(tbx);
}

void _pressedChar(Textbox tbx, char chr) {
    if (tbx->lastCharIdx>=tbx->maxInputChars) return;
    if (tbx->lastCharIdx>=tbx->textSize-1) _doubleTextBuffer(tbx);
    if (tbx->cursorIdx == tbx->lastCharIdx) {
        (tbx->text)[(tbx->lastCharIdx)++] = chr;
        (tbx->text)[++(tbx->cursorIdx)] = 0;
    } else {
        (tbx->text)[tbx->lastCharIdx+1] = 0;
        for (int i=(tbx->lastCharIdx++); i>tbx->cursorIdx; i--) tbx->text[i]=tbx->text[i-1];
        (tbx->text)[(tbx->cursorIdx)++] = chr;
    }
}

void _pressedBackspace(Textbox tbx) {
    if (tbx->cursorIdx>0) _removeCharAtIdx(tbx, --(tbx->cursorIdx));
}

void _pressedDelete(Textbox tbx) {
    if (tbx->cursorIdx<tbx->lastCharIdx) _removeCharAtIdx(tbx, tbx->cursorIdx);
}

int _isCharValidForTitle(char n) {
    return n>=32 && n<=126;
}

int _isCharValidForPosInt(char n) {
    if (n<'0' || n>'9') return 0;
    return 1;
}

int _isValidString(Textbox tbx, int size, const char* text) {
    if (size>tbx->maxInputChars) return 0;
    if (tbx->inputType == T_IN_STRING) {
        for (int i=0; i<size; i++) if (!_isCharValidForTitle(text[i])) return 0;
    } else if (tbx->inputType == T_IN_POSITIVE_INTEGER) {
        for (int i=0; i<size; i++) if (!_isCharValidForPosInt(text[i])) return 0;
    }
    return 1;
}

void _loadText(Textbox tbx, const char* text) {
    int size = strlen(text);
    if (!_isValidString(tbx, size, text)) return;
    char* newText = malloc((size+1)*sizeof(char));
    if (!newText) return;

    strncpy(newText, text, size+1);
    free(tbx->text);
    tbx->text = newText;
    tbx->textSize = size+1;
    tbx->lastCharIdx = size;
    tbx->cursorIdx = size;
}

void textboxLoadText(Textbox tbx, const char* text) {
    if (!tbx || !text) return;
    _loadText(tbx, text);
}

void textboxLoadPositiveInt(Textbox tbx, int pint) {
    if (!tbx || pint<=0) return;
    memset(tbx->text, 0, sizeof(char));

    char numBuff[12] = {0};
    snprintf(numBuff, sizeof(numBuff), "%d", pint);
    _loadText(tbx, numBuff);
}


int _allowedCharInputForCurrentState(Textbox tbx, char n) {
    if (!tbx) return 0;
    if (tbx->inputType == T_IN_STRING) return _isCharValidForTitle(n);
    else if (tbx->inputType == T_IN_POSITIVE_INTEGER) return _isCharValidForPosInt(n);
    return 0;
}

void _moveCursor(Textbox tbx, int d) {
    tbx->cursorIdx += d;
    if (tbx->cursorIdx>tbx->lastCharIdx) tbx->cursorIdx=tbx->lastCharIdx;
    if (tbx->cursorIdx<0) tbx->cursorIdx=0;
}

int textboxGetCursorIdx(Textbox tbx) {
    if (!tbx) return -1;
    return tbx->cursorIdx;
}



int _isKeyRepeating(struct key_press_state* kps, int key) {
    if (IsKeyPressed(key)) {
        kps->timer = 0;
        return 1;
    }
    if (IsKeyDown(key)) {
        kps->timer += GetFrameTime();
        if (kps->timer >= kps->delay) {
            kps->timer -= kps->interval;
            return 1;
        }
    }
    return 0;
}

void textboxUpdateText(Textbox tbx) {
    if (!tbx) return;

    int key = GetCharPressed();
    while (key>0) {
        if (_allowedCharInputForCurrentState(tbx, key)) _pressedChar(tbx, key);
        key = GetCharPressed();
    }

    if (_isKeyRepeating(&(tbx->backspace), KEY_BACKSPACE)) _pressedBackspace(tbx);
    if (_isKeyRepeating(&(tbx->delete), KEY_DELETE)) _pressedDelete(tbx);
    _moveCursor(tbx, _isKeyRepeating(&(tbx->right), KEY_RIGHT)-_isKeyRepeating(&(tbx->left), KEY_LEFT));
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        if (tbx->state.focused) tbx->justUnfocused=1;
        tbx->state.focused = 0;
    }
}

const char* textboxGetText(Textbox tbx) {
    if (!tbx) return NULL;
    return tbx->text;
}

int textboxGetPositiveInt(Textbox tbx) {
    if (!tbx || tbx->inputType != T_IN_POSITIVE_INTEGER) return 0;
    if (!*(tbx->text)) return 0;
    int num=0, idx=0;
    char n;
    while ((n=(tbx->text)[idx++])) {
        if (n<'0' || n>'9') return 0;
        num=10*num+n-'0';
    }
    return num;
}

char* textboxGetTextBeforeCursor(Textbox tbx) {
    if (!tbx || tbx->cursorIdx<0) return NULL;

    char* new = malloc((tbx->cursorIdx+1)*sizeof(char));
    if (!new) return NULL;

    if (tbx->cursorIdx) strncpy(new, tbx->text, tbx->cursorIdx);
    new[tbx->cursorIdx] = 0;
    return new;     // Free on your own
}

void textboxSetMaxChars(Textbox tbx, int maxChars) {
    if (!tbx || maxChars<=0) return;

    tbx->maxInputChars = maxChars;

    if (maxChars<tbx->lastCharIdx) {
        while (maxChars<tbx->lastCharIdx) _removeLastChar(tbx);
    }
}

void textboxUpdateRectangle(Textbox tbx, Rectangle rect) {
    if (!tbx) return;
    tbx->rect = rect;
}

void textboxUpdateCursorOnHover(Textbox tbx, int cursor) {
    if (!tbx) return;
    tbx->cursorOnHover = cursor;
}

Rectangle textboxGetRectangle(Textbox tbx) {
    if (!tbx) return (Rectangle){0,0,0,0};
    return tbx->rect;
}

void textboxUpdateRoundness(Textbox tbx, float roundness) {
    if (!tbx) return;
    tbx->roundness = roundness;
}

float textboxGetRoundness(Textbox tbx) {
    if (!tbx) return 0;
    return tbx->roundness;
}

int isTextboxJustUnfocused(Textbox tbx) {
    if (!tbx) return 0;
    return tbx->justUnfocused;
}

int isTextboxClicked(Textbox tbx) {
    if (!tbx) return 0;
    return tbx->state.pressed;
}

int isTextboxHovered(Textbox tbx) {
    if (!tbx) return 0;
    return tbx->state.hovered;
}

int isTextboxFocused(Textbox tbx) {
    if (!tbx) return 0;
    return tbx->state.focused;
}

int isTextboxDragged(Textbox tbx) {
    if (!tbx) return 0;
    return tbx->state.dragging;
}

float textboxGetEffectValue(Textbox tbx) {
    if (!tbx) return 0;
    return tbx->effect;
}

void textboxSetEffectTarget(Textbox tbx, int tar) {
    if (!tbx || (tar!=0 && tar!=1)) return;
    tbx->state.effectTarget = tar;
}