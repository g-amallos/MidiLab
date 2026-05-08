#include <stdlib.h>
#include "ui_internal.h"
#include <interface.h>
#include <handler.h>
#include <utils.h>




typedef struct ui_slider {
    Rectangle rect;
    struct ui_element_interaction_values state;
    uint8_t effectSpeed;
    uint8_t canBeShadowedByLayout;
    
    float value;

    int cursorOnHover;
    float roundness;
    float effect;
} *Slider;


Slider sliderCreate(Rectangle rect, float roundness) {
    Slider sld = malloc(sizeof(struct ui_slider));
    if (!sld) return NULL;
    sld->rect = rect;

    sld->state.disabled = 0;
    sld->state.hovered = 0;
    sld->state.dragging = 0;
    sld->state.pressed = 0;
    sld->state.effectTarget = 0;
    sld->state.disableHover = 0;

    sld->canBeShadowedByLayout = 1;
    sld->value = 0;
    
    sld->cursorOnHover = MOUSE_CURSOR_POINTING_HAND;
    sld->roundness = roundness;
    sld->effectSpeed = 4;
    sld->effect = 0;

    return sld;
}

void sliderFree(Slider sld) {
    if (!sld) return;
    free(sld);
}

void updateSliderEffect(Slider sld) {
    sld->effect += (sld->state.effectTarget-sld->effect)/sld->effectSpeed;
    if (!(sld->state.disabled) && (sld->state.hovered || sld->state.dragging)) setNextMouseCursor(sld->cursorOnHover);
}

void sliderUpdate(Slider sld, int effectTarget) {
    if (!sld) return;
    if (sld->state.disabled) {
        sld->state.effectTarget = 0;
        sld->state.hovered = 0;
        sld->state.pressed = 0;
        sld->state.dragging = 0;
        updateSliderEffect(sld);
        return;
    }
    sld->state.hovered = (!(sld->state.disableHover)) && (!(sld->canBeShadowedByLayout && UIisHoveringOverLayout()) && checkCollisionPointRoundedRect(globalMouseHandler.pos, sld->rect, sld->roundness));
    

    if (globalMouseHandler.pressed) {
        if (sld->state.hovered) {
            sld->state.pressed = 1;
            sld->state.dragging = 1;
        } else {
            sld->state.pressed = 0;
            sld->state.dragging = 0;
        }
    } else sld->state.pressed = 0;
    
    if (globalMouseHandler.released || !globalMouseHandler.down) {
        sld->state.dragging = 0;
    }

    if (effectTarget==1) sld->state.effectTarget=1;
    else if (effectTarget==0) sld->state.effectTarget=0;
    else sld->state.effectTarget = (!(sld->state.disabled) && ((sld->state.hovered && (!globalMouseHandler.down || globalMouseHandler.pressed)) || sld->state.dragging || sld->state.pressed));

    updateSliderEffect(sld);

}

void sliderDisable(Slider sld) {
    if (!sld) return;
    sld->state.disabled = 1;
    sld->state.hovered = 0;
    sld->state.dragging = 0;
    sld->state.pressed = 0;
    sld->state.effectTarget = 0;
}

void sliderEnable(Slider sld) {
    if (!sld) return;
    sld->state.disabled = 0;
}


void sliderDisableHover(Slider sld) {
    if (!sld) return;
    sld->state.disableHover = 1;
}

void sliderEnableHover(Slider sld) {
    if (!sld) return;
    sld->state.disableHover = 0;
}

void sliderEnableLayoutShadowing(Slider sld) {
    if (!sld) return;
    sld->canBeShadowedByLayout = 1;
}

void sliderDisableLayoutShadowing(Slider sld) {
    if (!sld) return;
    sld->canBeShadowedByLayout = 0;
}

void sliderUpdateRectangle(Slider sld, Rectangle rect) {
    if (!sld) return;
    sld->rect = rect;
}

void sliderUpdateCursorOnHover(Slider sld, int cursor) {
    if (!sld) return;
    sld->cursorOnHover = cursor;
}

Rectangle sliderGetRectangle(Slider sld) {
    if (!sld) return (Rectangle){0,0,0,0};
    return sld->rect;
}

void sliderUpdateRoundness(Slider sld, float roundness) {
    if (!sld) return;
    sld->roundness = roundness;
}

float sliderGetRoundness(Slider sld) {
    if (!sld) return 0;
    return sld->roundness;
}

int isSliderClicked(Slider sld) {
    if (!sld) return 0;
    return sld->state.pressed;
}

int isSliderHovered(Slider sld) {
    if (!sld) return 0;
    return sld->state.hovered;
}

int isSliderDragged(Slider sld) {
    if (!sld) return 0;
    return sld->state.dragging;
}

float sliderGetEffectValue(Slider sld) {
    if (!sld) return 0;
    return sld->effect;
}

void sliderSetEffectTarget(Slider sld, int tar) {
    if (!sld || (tar!=0 && tar!=1)) return;
    sld->state.effectTarget = tar;
}

float sliderGetSlideValue(Slider sld) {
    if (!sld) return 0;
    return sld->value;
}

void sliderUpdateSlideValue(Slider sld, float val) {
    if (!sld || val<0 || val>1) return;
    sld->value = val;
}

float sliderHelperMinMaxToNormalizedLinear(float val, float min, float max) {
    if (max==min) return 0;
    return floatClip((val-min)/(max-min), 0, 1);
}

float sliderHelperNormalizedToMinMaxLinear(float normalized, float min, float max) {
    return min+(max-min)*normalized;
}

float sliderUpdateValueCommonHorizontal(Slider sld) {
    if (!sld) return 0;
    float val = sliderHelperMinMaxToNormalizedLinear(globalMouseHandler.pos.x, sld->rect.x, sld->rect.x+sld->rect.width);
    sliderUpdateSlideValue(sld, val);
    return val;
}

Rectangle sliderGetRectangleValueCommon(Slider sld) {
    if (!sld) return (Rectangle){0,0,0,0};
    return (Rectangle){sld->rect.x, sld->rect.y, sld->value*sld->rect.width, sld->rect.height};
}

Vector2 sliderGetPosValueCommon(Slider sld) {
    if (!sld) return (Vector2){0,0};
    return (Vector2){sld->rect.x+sld->value*sld->rect.width, sld->rect.y+0.5*sld->rect.height};
}