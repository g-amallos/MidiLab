#include <interface.h>
#include <backend.h>
#include <raymath.h>
#include <colors.h>
#include <utils.h>
#include <ui.h>
#include <stdlib.h>
#include <stdio.h>
#include <tinyfiledialogs.h>
#include <images.h>

#define DEFAULT_PROJECT_TITLE "Untitled Project (1)"



Button openFileButton = NULL;
ButtonList layoutButton = NULL;


void controlLineInit() {
    openFileButton = buttonCreate((Rectangle){20, 20, 80, 30}, 0.25);
    buttonUpdateCursorOnHover(openFileButton, MOUSE_CURSOR_POINTING_HAND);
}

void controlLineClose() {
    if (openFileButton) buttonFree(openFileButton);
    openFileButton = NULL;
}




void updateControlLineButtons(float lineHeight) {
    Rectangle rect = {0.2*lineHeight, 0.2*lineHeight, 0.6*lineHeight, 0.6*lineHeight};
    buttonUpdateRectangle(openFileButton, rect);
    int target = (layoutButton)?1:-1;
    buttonUpdate(openFileButton, target);
}



void openFileButtonAction() {
    char const * lFilterPatterns[2] = { "*.mid", "*.midi" };

    char const * lSelected = tinyfd_openFileDialog(
        "Open MIDI Project",      // Title
        "",                       // Default path (use "" or NULL for current directory)
        2,                        // Number of filter patterns
        lFilterPatterns,          // Filter patterns array
        "MIDI Files",             // Description of the filter
        0                         // Allow multiple select (0 = No)
    );

    if (lSelected) {
        printf("`%s`\n", lSelected);
    }
}

void createBaseLayout(float lineHeight) {
    if (layoutButton) buttonListFree(layoutButton, 1);
    Rectangle brect = {0.2*lineHeight, 0.9*lineHeight, floatMax(2*lineHeight, 120), floatMax(3*lineHeight, 180)};
    float spacing = floatMax(0.08*lineHeight, 4.8);
    layoutButton = buttonListCreate(brect, 5, 0.18, spacing, 1);
    if (!layoutButton) return;
    int btns = buttonListGetNum(layoutButton);
    for (int i=0; i<btns; i++) {
        buttonUpdateCursorOnHover(buttonListGetButtonAt(layoutButton, i), MOUSE_CURSOR_POINTING_HAND);
    }
}

void destroyBaseLayout() {
    if (!layoutButton) return;
    buttonListFree(layoutButton, 1);
    layoutButton = NULL;
}

void updateBaseLayout(float lineHeight) {
    if (!layoutButton) return;
    Rectangle brect = {0.2*lineHeight, 0.9*lineHeight, floatMax(2*lineHeight, 120), floatMax(3*lineHeight, 180)};
    float spacing = floatMax(0.08*lineHeight, 4.8);
    buttonListUpdateRect(layoutButton, brect);
    buttonListUpdateSpacing(layoutButton, spacing);
    buttonListUpdate(layoutButton);
    if (buttonListShouldDelete(layoutButton)) destroyBaseLayout();
}

void renderClickableButton(Button btn, const char* text, Vector2 textAlign, Vector2 textOffset, float textSize) {
    if (!btn) return;
    Color col1 = {20, 21, 23, 255}, col2={34, 36, 43, 255};
    float effect = buttonGetEffectValue(btn);
    Color blend1 = blendColors(col1, col2, effect);
    Rectangle rect = buttonGetRectangle(btn);
    DrawRectangleRounded(rect, buttonGetRoundness(btn), 8, blend1);
    Color blend2 = blendColors(COLOR_TEXT_4, COLOR_TEXT_3, effect);
    DrawRectangleRoundedLinesEx(rect, buttonGetRoundness(btn), 8, lerp(1, 2, effect), blend2);
    Vector2 tarPos = lerpVector2_vec((Vector2){rect.x, rect.y}, (Vector2){rect.x+rect.width, rect.y+rect.height}, textAlign);
    renderFontStringAlign(GlobalFonts[0].font, text, Vector2Add(tarPos, textOffset), textAlign, textSize, 0, COLOR_TEXT_1);
}


void renderBaseLayoutButton(Button btn, const char* text, Vector2 textAlign, Vector2 textOffset, float textSize, enum icon_title icon) {
    if (!btn) return;
    Color col1 = {20, 21, 23, 255}, col2={34, 36, 43, 255};
    float effect = buttonGetEffectValue(btn);
    Color blend1 = blendColors(col1, col2, effect);
    Rectangle rect = buttonGetRectangle(btn);
    DrawRectangleRounded(rect, buttonGetRoundness(btn), 8, blend1);
    Color blend2 = blendColors(COLOR_TEXT_4, COLOR_TEXT_3, effect);
    DrawRectangleRoundedLinesEx(rect, buttonGetRoundness(btn), 8, lerp(1, 2, effect), blend2);
    Vector2 tarPos = lerpVector2_vec((Vector2){rect.x, rect.y}, (Vector2){rect.x+rect.width, rect.y+rect.height}, textAlign);
    renderFontStringAlign(GlobalFonts[0].font, text, Vector2Add(tarPos, textOffset), textAlign, textSize, 0, COLOR_TEXT_1);
    if (effect>1e-5) {
        Color tar =COLOR_TEXT_1;
        tar.a = 0;
        Color blend3 = blendColors(tar, COLOR_TEXT_1, effect);
        Rectangle targRect = {rect.x+(0.75+0.25*effect)*rect.width-0.7*rect.height, rect.y+0.3*rect.height, 0.4*rect.height, 0.4*rect.height};
        iconRerder(icon, targRect, blend3);
    }
}

void renderBaseLayout() {
    if (!layoutButton) return;
    Rectangle brect = buttonListGetRect(layoutButton);
    Color col1 = {20, 21, 23, 255};
    float roundness = buttonListGetRoundness(layoutButton);
    DrawRectangleRounded(brect, roundness, 8, col1);
    const char* texts[] = {"Project", "Edit", "View", "Settings", "Export"};
    int num = buttonListGetNum(layoutButton);
    for (int i=0; i<num; i++) renderBaseLayoutButton(buttonListGetButtonAt(layoutButton, i), texts[i], (Vector2){0, 0.5}, (Vector2){10, 0}, brect.height*0.08, T_ICON_RIGHT);
    DrawRectangleRoundedLinesEx(brect, roundness, 8, 1, COLOR_TEXT_4);
}



void renderControlLine() {

    float lineHeight = floatMin(0.08*screenSize.y, 100);
    updateControlLineButtons(lineHeight);



    DrawRectangle(0, 0, (int)(screenSize.x+2), (int)(lineHeight), COLOR_CONTROL_LINE_BACKGROUND);

    char* projectTitle = projectGetCurrentTitle();
    if (!projectTitle) projectTitle = DEFAULT_PROJECT_TITLE;

    renderFontStringAlign(GlobalFonts[0].font, projectTitle, (Vector2){.x=screenSize.x*0.5, .y=0.5*lineHeight}, (Vector2){.x=0.5, .y=0.5}, lineHeight*0.5, 0, COLOR_TEXT_1);


    Color col1 = {20, 21, 23, 255}, col2={34, 36, 43, 255};
    Color blendedCol = blendColors(col1, col2, buttonGetEffectValue(openFileButton));
    Rectangle rect = buttonGetRectangle(openFileButton);
    DrawRectangleRounded(rect, buttonGetRoundness(openFileButton), 8, blendedCol);
    iconRerder(T_ICON_MENU, scaleRctangleFromCenter(rect, lerp(0.8, 1, buttonGetEffectValue(openFileButton))), COLOR_TEXT_1);
    if (isButtonClicked(openFileButton)) createBaseLayout(lineHeight); //openFileButtonAction();



    DrawLineEx((Vector2){0, lineHeight}, (Vector2){screenSize.x, lineHeight}, 2, COLOR_TEXT_4);
    
    float gradientHeight = 0.1*lineHeight;
    Color topCol = {120, 139, 179, 255}, bottomCol={160, 160, 160, 0};
    DrawRectangleGradientV(0, (int)lineHeight, (int)(screenSize.x+2), (int)gradientHeight, topCol, bottomCol);

    if (layoutButton) updateBaseLayout(lineHeight);
    if (layoutButton) renderBaseLayout(lineHeight);
}