#include <stdlib.h>
#include "ui_internal.h"
#include <interface.h>
#include <handler.h>



typedef struct ui_button {
    Rectangle rect;
    struct ui_element_interaction_values state;
    uint8_t canBeShadowedByLayout;
    uint8_t disabledByFrontLayout;
    // Can add 2 bytes here

    int cursorOnHover;
    float roundness;
    float effect;
    float effectSpeed;
} *Button;


void updateButtonEffect(Button btn) {
    btn->effect += (btn->state.effectTarget-btn->effect)*(btn->effectSpeed);
    if (!(btn->state.disabled) && (btn->state.hovered || btn->state.dragging)) setNextMouseCursor(btn->cursorOnHover);
}


void _updateButtonDisabled(Button btn) {
    if (!btn) return;
    btn->state.effectTarget = 0;
    btn->state.hovered = 0;
    btn->state.pressed = 0;
    btn->state.dragging = 0;
    btn->state.released = 0;
    updateButtonEffect(btn);
}

void _updateButtonState(Button btn) {
    btn->state.released = 0;
    if (globalMouseHandler.pressed) {
        if (btn->state.hovered) {
            btn->state.pressed = 1;
            btn->state.dragging = 1;
        } else {
            btn->state.pressed = 0;
            btn->state.dragging = 0;
        }
    } else btn->state.pressed = 0;
    
    if (globalMouseHandler.released || !globalMouseHandler.down) {
        btn->state.released = btn->state.dragging;
        btn->state.dragging = 0;
    }
}

void buttonUpdate(Button btn, int effectTarget) {
    if (!btn) return;
    if (btn->state.disabled) {
        _updateButtonDisabled(btn);
        return;
    }
    btn->state.hovered = (!(btn->state.disableHover)) && (!(btn->disabledByFrontLayout && UIexistsFrontLayoutOverlay())) &&(!(btn->canBeShadowedByLayout && UIisHoveringOverLayout()) && checkCollisionPointRoundedRect(globalMouseHandler.pos, btn->rect, btn->roundness));
    
    _updateButtonState(btn);

    if (effectTarget==1) btn->state.effectTarget=1;
    else if (effectTarget==0) btn->state.effectTarget=0;
    else btn->state.effectTarget = (!(btn->state.disabled) && ((btn->state.hovered && (!globalMouseHandler.down || globalMouseHandler.pressed)) || btn->state.dragging || btn->state.pressed));

    updateButtonEffect(btn);

}


void buttonUpdateCustomHover(Button btn, int hover) {
    if (!btn) return;
    if (btn->state.disabled) {
        _updateButtonDisabled(btn);
        return;
    }
    btn->state.hovered = hover;
    _updateButtonState(btn);

    btn->state.effectTarget = (!(btn->state.disabled) && ((btn->state.hovered && (!globalMouseHandler.down || globalMouseHandler.pressed)) || btn->state.dragging || btn->state.pressed));
    updateButtonEffect(btn);
}

void buttonUpdateCustomHoverEffect(Button btn, int hover, int effectTarget) {
    if (!btn) return;
    if (btn->state.disabled) {
        _updateButtonDisabled(btn);
        return;
    }
    btn->state.hovered = hover;
    _updateButtonState(btn);

    if (effectTarget==1) btn->state.effectTarget=1;
    else if (effectTarget==0) btn->state.effectTarget=0;
    else btn->state.effectTarget = (!(btn->state.disabled) && ((btn->state.hovered && (!globalMouseHandler.down || globalMouseHandler.pressed)) || btn->state.dragging || btn->state.pressed));
    updateButtonEffect(btn);
}

void buttonSetCurrentEffect(Button btn, float effect) {
    if (!btn || effect<0 || effect>1) return;
    btn->effect = effect;
}

void buttonDisable(Button btn) {
    if (!btn) return;
    btn->state.disabled = 1;
    btn->state.hovered = 0;
    btn->state.released = (btn->state.dragging);
    btn->state.dragging = 0;
    btn->state.pressed = 0;
    btn->state.effectTarget = 0;
}

void buttonEnable(Button btn) {
    if (!btn) return;
    btn->state.disabled = 0;
}

Button buttonCreate(Rectangle rect, float roundness) {
    Button btn = malloc(sizeof(struct ui_button));
    if (!btn) return NULL;
    btn->rect = rect;

    btn->state.disabled = 0;
    btn->state.hovered = 0;
    btn->state.dragging = 0;
    btn->state.pressed = 0;
    btn->state.effectTarget = 0;
    btn->state.disableHover = 0;
    btn->state.released = 0;

    btn->canBeShadowedByLayout = 1;
    btn->disabledByFrontLayout = 1;
    
    btn->cursorOnHover = MOUSE_CURSOR_POINTING_HAND;
    btn->roundness = roundness;
    btn->effectSpeed = 0.25;
    btn->effect = 0;

    return btn;
}

void buttonFree(Button btn) {
    if (!btn) return;
    free(btn);
}

void buttonSetEffectSpeed(Button btn, float effectSpeed) {
    if (effectSpeed<=0 || effectSpeed>1 || !btn) return;
    btn->effectSpeed = effectSpeed;
}

void buttonEnableLayoutShadowing(Button btn) {
    if (!btn) return;
    btn->canBeShadowedByLayout = 1;
}

void buttonDisableLayoutShadowing(Button btn) {
    if (!btn) return;
    btn->canBeShadowedByLayout = 0;
}

void buttonEnableOnFrontLayout(Button btn) {
    if (!btn) return;
    btn->disabledByFrontLayout = 0;
}

void buttonDisableOnFrontLayout(Button btn) {
    if (!btn) return;
    btn->disabledByFrontLayout = 1;
}

void buttonDisableHover(Button btn) {
    if (!btn) return;
    btn->state.disableHover = 1;
}

void buttonEnableHover(Button btn) {
    if (!btn) return;
    btn->state.disableHover = 0;
}

void buttonUpdateRectangle(Button btn, Rectangle rect) {
    if (!btn) return;
    btn->rect = rect;
}

void buttonUpdateCursorOnHover(Button btn, int cursor) {
    if (!btn) return;
    btn->cursorOnHover = cursor;
}

Rectangle buttonGetRectangle(Button btn) {
    if (!btn) return (Rectangle){0,0,0,0};
    return btn->rect;
}

void buttonUpdateRoundness(Button btn, float roundness) {
    if (!btn) return;
    btn->roundness = roundness;
}

float buttonGetRoundness(Button btn) {
    if (!btn) return 0;
    return btn->roundness;
}

int isButtonClicked(Button btn) {
    if (!btn) return 0;
    return btn->state.pressed;
}

int isButtonHovered(Button btn) {
    if (!btn) return 0;
    return btn->state.hovered;
}

int isButtonDragged(Button btn) {
    if (!btn) return 0;
    return btn->state.dragging;
}

int isButtonReleased(Button btn) { 
    if (!btn) return 0;
    return btn->state.released;
}

float buttonGetEffectValue(Button btn) {
    if (!btn) return 0;
    return btn->effect;
}

void buttonSetEffectTarget(Button btn, int tar) {
    if (!btn || (tar!=0 && tar!=1)) return;
    btn->state.effectTarget = tar;
}