#ifndef UI_H
#define UI_H

#include <raylib.h>
#include <stdint.h>




enum ui_element_types {
    ELEMENT_BUTTON,
    ELEMENT_SLIDER,
    ELEMENT_TEXTBOX,

    ELEMENT_END
};

typedef struct ui_button *Button;


Button buttonCreate(Rectangle rect, float roundness);
void buttonFree(Button btn);

void buttonEnable(Button btn);
void buttonDisable(Button btn);

void buttonUpdateRectangle(Button btn, Rectangle rect);
void buttonUpdateRoundness(Button btn, float roundness);
void buttonUpdate(Button btn, int effectTarget);
int isButtonClicked(Button btn);
int isButtonHovered(Button btn);
int isButtonDragged(Button btn);
float buttonGetEffectValue(Button btn);
Rectangle buttonGetRectangle(Button btn);
float buttonGetRoundness(Button btn);
void buttonUpdateCursorOnHover(Button btn, int cursor);
void buttonSetEffectTarget(Button btn, int tar);




typedef struct ui_button_list *ButtonList;

ButtonList buttonListCreate(Rectangle rect, int num, float roundness, float spacing, int createButtons);
int buttonListGetNum(ButtonList btnList);
void buttonListUpdateRect(ButtonList btnList, Rectangle rect);
Rectangle buttonListGetRect(ButtonList btnList);
void buttonListUpdateRoundness(ButtonList btnList, float roundness);
float buttonListGetRoundness(ButtonList btnList);
void buttonListUpdateSpacing(ButtonList btnList, float spacing);
void buttonListUpdateButtonsRects(ButtonList btnList);
void buttonListUpdateButtons(ButtonList btnList);
void buttonListSetButtonAt(ButtonList btnList, Button btn, int index, int freeExisting);
Button buttonListGetButtonAt(ButtonList btnList, int index);
void buttonListAppendButton(ButtonList btnList, Button btn);
void buttonListFree(ButtonList btnList, int freeButtons);
void buttonListUpdate(ButtonList btnList);
int buttonListShouldDelete(ButtonList btnList);





typedef struct ui_textbox *Textbox;
enum textbox_input_type {
    T_IN_STRING,
    T_IN_POSITIVE_INTEGER
};

void textboxUpdate(Textbox tbx, int effectTarget);
void textboxDisable(Textbox tbx);
void textboxEnable(Textbox tbx);
Textbox textboxCreate(Rectangle rect, float roundness, enum textbox_input_type inputType, int maxInputChars);
const char* textboxGetText(Textbox tbx);
void textboxLoadText(Textbox tbx, const char* text);
void textboxFree(Textbox tbx);
void textboxUpdateText(Textbox tbx);
void textboxUpdateRectangle(Textbox tbx, Rectangle rect);
void textboxUpdateCursorOnHover(Textbox tbx, int cursor);
Rectangle textboxGetRectangle(Textbox tbx);
void textboxUpdateRoundness(Textbox tbx, float roundness);
float textboxGetRoundness(Textbox tbx);
int isTextboxClicked(Textbox tbx);
int isTextboxHovered(Textbox tbx);
int isTextboxFocused(Textbox tbx);
int isTextboxDragged(Textbox tbx);
float textboxGetEffectValue(Textbox tbx);
void textboxSetEffectTarget(Textbox tbx, int tar);
int textboxGetCursorIdx(Textbox tbx);
void textboxSetMaxChars(Textbox tbx, int maxChars);
char* textboxGetTextBeforeCursor(Textbox tbx);                  // User's got to free this
int textboxGetPositiveInt(Textbox tbx);
int isTextboxJustUnfocused(Textbox tbx);
void textboxLoadPositiveInt(Textbox tbx, int pint);



#endif