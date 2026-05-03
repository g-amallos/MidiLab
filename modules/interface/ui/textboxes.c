#include <stdlib.h>
#include "ui_internal.h"
#include <interface.h>
#include <handler.h>



typedef struct ui_slider {
    Rectangle rect;
    struct ui_element_interaction_values state;
    uint8_t effectSpeed;
    
    // Can add 2 bytes here

    int cursorOnHover;
    float roundness;
    float effect;
} *Slider;


