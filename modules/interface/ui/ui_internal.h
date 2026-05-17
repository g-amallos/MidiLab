#ifndef UI_INTERNAL_H
#define UI_INTERNAL_H

#include <ui.h>


struct ui_element_interaction_values {
    uint8_t disabled: 1;
    uint8_t hovered: 1;
    uint8_t pressed: 1;
    uint8_t dragging: 1;
    uint8_t focused: 1;      
    uint8_t effectTarget: 1;
    uint8_t disableHover: 1;
    uint8_t released: 1;
} __attribute__((packed));



void UIinTextInput();
void UIhoveringOverLayout();


#endif