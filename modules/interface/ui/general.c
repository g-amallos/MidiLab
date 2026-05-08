#include "ui_internal.h"


int _UIisInTextInput = 0;
int _UIhoveringOverLayout = 0;



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