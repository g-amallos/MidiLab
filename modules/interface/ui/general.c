#include "ui_internal.h"
#include <interface.h>


int _UIisInTextInput = 0;
int _UIhoveringOverLayout = 0;
int _UIfrontLayoutOverlay = 0;
float _UIsemitransparentOverlay = 0;


int UIisInTextInput() {
    return _UIisInTextInput;
}

int UIisHoveringOverLayout() {
    return _UIhoveringOverLayout;
}


void UIiterationReset() {
    _UIisInTextInput=0;
    _UIhoveringOverLayout=0;
}

void UIinTextInput() {
    _UIisInTextInput = 1;
}

void UIhoveringOverLayout() {
    _UIhoveringOverLayout=1;
}

void UIcreateFrontLayoutOverlay() {
    _UIfrontLayoutOverlay=1;
}

void UIdestroyFrontLayoutOverlay() {
    _UIfrontLayoutOverlay=0;
}

int UIexistsFrontLayoutOverlay() {
    return _UIfrontLayoutOverlay;
}

void UIupdateTransparentOverlay() {
    _UIsemitransparentOverlay += 0.15*(_UIfrontLayoutOverlay-_UIsemitransparentOverlay);
    DrawRectangle(0,0,screenSize.x,screenSize.y, (Color){0,0,0,(unsigned char)(120*_UIsemitransparentOverlay)});
}