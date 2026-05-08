#include <stdlib.h>
#include "ui_internal.h"
#include <interface.h>
#include <handler.h>
#include <stdio.h>


typedef struct ui_button_list {
    Rectangle rect;
    uint8_t toDelete;
    uint8_t ignoreDeletion;
    int numButtons;
    float roundness;
    float spacing;
    Button* buttonArr;
} *ButtonList;



ButtonList buttonListCreate(Rectangle rect, int num, float roundness, float spacing, int createButtons) {
    if (num<=0) return NULL;
    ButtonList btnList = malloc(sizeof(struct ui_button_list));
    if (!btnList) return NULL;
    Button* btns = calloc(num, sizeof(Button));
    if (!btns) {
        free(btnList);
        return NULL;
    }

    if (createButtons) {
        for (int i=0; i<num; i++) {
            btns[i] = buttonCreate((Rectangle){0,0,1,1}, roundness);
            if (!(btns[i])) {
                for (int j=0; j<i; j++) buttonFree(btns[j]);
                free(btns);
                free(btnList);
                return NULL;
            }
            buttonDisableLayoutShadowing(btns[i]);
        }
    }
    btnList->toDelete = 0;
    btnList->ignoreDeletion = 1;
    btnList->rect = rect;
    btnList->buttonArr = btns;
    btnList->numButtons = num;
    btnList->roundness = roundness;
    btnList->spacing = spacing;

    return btnList;
}

int buttonListGetNum(ButtonList btnList) {
    if (!btnList) return 0;
    return btnList->numButtons;
}

void buttonListUpdateRect(ButtonList btnList, Rectangle rect) {
    if (!btnList) return;
    btnList->rect = rect;
}

Rectangle buttonListGetRect(ButtonList btnList) {
    if (!btnList) return (Rectangle){0,0,0,0};
    return btnList->rect;
}

void buttonListUpdateRoundness(ButtonList btnList, float roundness) {
    if (!btnList) return;
    btnList->roundness = roundness;
}

float buttonListGetRoundness(ButtonList btnList) {
    if (!btnList) return 0;
    return btnList->roundness;
}

void buttonListUpdateSpacing(ButtonList btnList, float spacing) {
    if (!btnList) return;
    btnList->spacing = spacing;
}

void buttonListUpdateButtonsRects(ButtonList btnList) {
    if (!btnList) return;

    float totalHeight = btnList->rect.height;
    float totalWidth = btnList->rect.width;

    float realHeight = totalHeight-btnList->spacing*(btnList->numButtons+1);
    float realWidth = totalWidth-btnList->spacing*2;
    float buttonHeight = realHeight/btnList->numButtons;

    float x=btnList->rect.x+btnList->spacing;
    float y=btnList->rect.y+btnList->spacing;
    float yOffset = buttonHeight+btnList->spacing;

    float pixRad = getRadiusForRoundedRectangle(btnList->rect, btnList->roundness);
    float targetRoundness = getRoundnessForRoundedRectangle((Rectangle){.x=x, .y=y, .width=realWidth, .height=buttonHeight}, pixRad-btnList->spacing);

    for (int i=0; i<btnList->numButtons; i++) {
        Button btn = (btnList->buttonArr)[i];
        if (!btn) continue;
        buttonUpdateRectangle(btn, (Rectangle){.x=x, .y=y, .width=realWidth, .height=buttonHeight});
        buttonUpdateRoundness(btn, targetRoundness);
        y+=yOffset;
    }
}


void buttonListUpdate(ButtonList btnList) {
    if (!btnList) return;
    int isTouchingBaseLayout = checkCollisionPointRoundedRect(globalMouseHandler.pos, btnList->rect, btnList->roundness);
    if (isTouchingBaseLayout) UIhoveringOverLayout();
    if (globalMouseHandler.pressed && !(btnList->ignoreDeletion)) btnList->toDelete |= !isTouchingBaseLayout;
    if (btnList->ignoreDeletion) btnList->ignoreDeletion = 0;
    buttonListUpdateButtonsRects(btnList);
    buttonListUpdateButtons(btnList);
}

int buttonListShouldDelete(ButtonList btnList) {
    if (!btnList) return -1;
    return btnList->toDelete;
}

void buttonListUpdateButtons(ButtonList btnList) {
    if (!btnList) return;
    for (int i=0; i<btnList->numButtons; i++) {
        Button btn = (btnList->buttonArr)[i];
        buttonUpdate(btn, -1);
    }
}

void buttonListSetButtonAt(ButtonList btnList, Button btn, int index, int freeExisting) {
    if (!btnList || !btn) return;
    if (btnList->numButtons>index && index>=0) {
        if (freeExisting) buttonFree((btnList->buttonArr)[index]);
        (btnList->buttonArr)[index] = btn;
        buttonDisableLayoutShadowing(btn);
    }
}

Button buttonListGetButtonAt(ButtonList btnList, int index) {
    if (!btnList) return NULL;
    if (btnList->numButtons>index && index>=0) {
        return (btnList->buttonArr)[index];
    }
    return NULL;
}

void buttonListAppendButton(ButtonList btnList, Button btn) {
    if (!btnList || !btn) return;
    Button* new = realloc(btnList->buttonArr, (btnList->numButtons+1)*sizeof(Button));
    if (!new) return;
    btnList->buttonArr = new;
    (btnList->buttonArr)[(btnList->numButtons)++]=btn;
    buttonDisableLayoutShadowing(btn);
}

void buttonListFree(ButtonList btnList, int freeButtons) {
    if (!btnList) return;
    if (freeButtons && btnList->buttonArr) {
        free(btnList->buttonArr);
        btnList->buttonArr = NULL;
        btnList->numButtons = 0;
    }
    free(btnList);
}

