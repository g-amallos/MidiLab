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
#define PROJECT_TITLE_PLACEHOLDER "Project Title"



Button openFileButton = NULL;
ButtonList layoutButton = NULL;
Textbox projectTitleTextbox=NULL, tempoTextbox=NULL;
Button previousButton=NULL, playPauseButton=NULL, nextButton=NULL;


void controlLineInit() {
    openFileButton = buttonCreate((Rectangle){20, 20, 80, 30}, 0.25);
    //buttonUpdateCursorOnHover(openFileButton, MOUSE_CURSOR_POINTING_HAND);

    projectTitleTextbox = textboxCreate((Rectangle){20, 20, 80, 30}, 0.25, T_IN_STRING, 30);
    textboxLoadText(projectTitleTextbox, projectGetCurrentTitle());

    tempoTextbox = textboxCreate((Rectangle){20, 20, 80, 30}, 0.25, T_IN_POSITIVE_INTEGER, 4);
    textboxLoadText(tempoTextbox, "120");

    previousButton = buttonCreate((Rectangle){20, 20, 80, 30}, 0.25);
    playPauseButton = buttonCreate((Rectangle){20, 20, 80, 30}, 0.25);
    nextButton = buttonCreate((Rectangle){20, 20, 80, 30}, 0.25);
}
void destroyBaseLayout();

void controlLineClose() {
    if (openFileButton) buttonFree(openFileButton);
    openFileButton = NULL;

    destroyBaseLayout();

    if (projectTitleTextbox) textboxFree(projectTitleTextbox);
    projectTitleTextbox = NULL;

    if (tempoTextbox) textboxFree(tempoTextbox);
    tempoTextbox = NULL;


    if (previousButton) buttonFree(previousButton);
    previousButton=NULL;
    if (playPauseButton) buttonFree(playPauseButton);
    playPauseButton=NULL;
    if (nextButton) buttonFree(nextButton);
    nextButton=NULL;
}



void renderProjectTextbox() {
    int isFocused = isTextboxFocused(projectTitleTextbox);
    const char* projectTitle = isFocused?textboxGetText(projectTitleTextbox):projectGetCurrentTitle();
    if (!projectTitle) projectTitle = DEFAULT_PROJECT_TITLE;

    float effect = textboxGetEffectValue(projectTitleTextbox);
    Rectangle rect = textboxGetRectangle(projectTitleTextbox);
    int cursorIdx = textboxGetCursorIdx(projectTitleTextbox);
    Vector2 tarPos = lerpVector2_vec((Vector2){rect.x, rect.y}, (Vector2){rect.x+rect.width, rect.y+rect.height}, (Vector2){0.5, 0.5});

    Color col1 = {20, 21, 23, 0}, col2={28, 30, 37, 255};
    Color blend1 = blendColors(col1, col2, effect);
    Color col3 = COLOR_TEXT_4; col3.a=0;
    Color blend2 = blendColors(col3, (isFocused)?COLOR_PALETTE_1_P6:COLOR_TEXT_4, effect);
    
    DrawRectangleRounded(rect, textboxGetRoundness(projectTitleTextbox), 8, blend1);
    if (isFocused) DrawRectangleRoundedLinesEx(rect, textboxGetRoundness(projectTitleTextbox), 8, lerp(0, floatMin(1+3*isFocused, 0.05*rect.height), effect), blend2);
    
    Color blend3 = blendColors(COLOR_PALETTE_1_P9, COLOR_TEXT_1, effect);
    if (*projectTitle) renderFontStringAlign(GlobalFonts[0].font, projectTitle, tarPos, (Vector2){0.5, 0.5}, rect.height*0.6, 0, blend3);
    else if (!isFocused) renderFontStringAlign(GlobalFonts[1].font, PROJECT_TITLE_PLACEHOLDER, tarPos, (Vector2){0.5, 0.5}, rect.height*0.6, 0, COLOR_TEXT_3);

    if (effect>1e-3) {
        Color c1=COLOR_TEXT_4; c1.a=(unsigned char)lerp(0, 150, effect);
        iconRerder(T_ICON_EDIT, scaleRctangleFromCenter((Rectangle){rect.x+rect.width, rect.y, rect.height, rect.height}, 0.35), c1);
    }

    if (isFocused && cursorIdx>=0) {        
        Color col = COLOR_TEXT_1;
        col.a = (unsigned char)trigInterpolation(255*effect, 0, fmod(1.1*GetTime(), 1.0));
        float cursorPosX;

        if (*projectTitle) {
            Vector2 totalSize = MeasureTextEx(GlobalFonts[0].font, projectTitle, rect.height*0.6, 0);
            char* partBeforeCursor = textboxGetTextBeforeCursor(projectTitleTextbox);
            Vector2 cursorOffset = MeasureTextEx(GlobalFonts[0].font, partBeforeCursor, rect.height*0.6, 0);
            free(partBeforeCursor);
            cursorPosX = tarPos.x - (totalSize.x * 0.5f) + cursorOffset.x;
        } else cursorPosX = tarPos.x;

        DrawLineEx((Vector2){cursorPosX, tarPos.y-0.2*rect.height}, (Vector2){cursorPosX, tarPos.y+0.2*rect.height}, 1, col);
    }
}

void renderTempoTextbox() {
    const char* tempoTxt = textboxGetText(tempoTextbox);
    int isFocused = isTextboxFocused(tempoTextbox);
    float effect = textboxGetEffectValue(tempoTextbox);
    float roundness = textboxGetRoundness(tempoTextbox);
    Rectangle rect = textboxGetRectangle(tempoTextbox);
    int cursorIdx = textboxGetCursorIdx(tempoTextbox);

    const char* tempoTxtShown=tempoTxt;
    if (!isFocused) tempoTxtShown=concatenateStrings(tempoTxt, " BPM");

    Vector2 tarPos = lerpVector2_vec((Vector2){rect.x, rect.y}, (Vector2){rect.x+rect.width, rect.y+rect.height}, (Vector2){0.5, 0.5});

    Color col2={28, 30, 37, 255};
    Color blend1 = blendColors(col2, COLOR_PALETTE_1_NUM_1, effect);
    
    DrawRectangleRounded(rect, roundness, 8, blend1);
    //if (isFocused) DrawRectangleRoundedLinesEx(rect, roundness, 8, lerp(0, floatMin(1+3*isFocused, 0.05*rect.height), effect), blend2);
    
    Color blend3 = blendColors(COLOR_PALETTE_1_P9, COLOR_TEXT_1, 1);
    renderFontStringAlign(GlobalFonts[0].font, tempoTxtShown, tarPos, (Vector2){0.5, 0.5}, rect.height*0.5, 0, blend3);

    if (isFocused && cursorIdx>=0) {        
        Color col = COLOR_TEXT_1;
        col.a = (unsigned char)trigInterpolation(255*effect, 0, fmod(1.1*GetTime(), 1.0));
        float cursorPosX;

        if (*tempoTxt) {
            Vector2 totalSize = MeasureTextEx(GlobalFonts[0].font, tempoTxt, rect.height*0.5, 0);
            char* partBeforeCursor = textboxGetTextBeforeCursor(tempoTextbox);
            Vector2 cursorOffset = MeasureTextEx(GlobalFonts[0].font, partBeforeCursor, rect.height*0.5, 0);
            free(partBeforeCursor);
            cursorPosX = tarPos.x - (totalSize.x * 0.5f) + cursorOffset.x;
        } else cursorPosX = tarPos.x;

        DrawLineEx((Vector2){cursorPosX, tarPos.y-0.2*rect.height}, (Vector2){cursorPosX, tarPos.y+0.2*rect.height}, 1, col);
    }
}

void renderPrevPlayPauseNext() {
    Button btns[] = {previousButton, playPauseButton, nextButton};
    enum icon_title btnIcons[] = {T_ICON_PREVIOUS, T_ICON_PLAY, T_ICON_NEXT};
    float sizes[] = {0.85, 0.65, 0.85};
    int btnNum = sizeof(btns)/sizeof(Button);

    Color col1 = {20, 21, 23, 0}, col2={28, 30, 34, 255};
    

    for (int i=0; i<btnNum; i++) {
        Button btn = btns[i];

        float effect = buttonGetEffectValue(btn);
        float roundness = buttonGetRoundness(btn);
        Rectangle rect = buttonGetRectangle(btn);
        
        Color blendedCol = blendColors(col1, col2, effect);
        
        DrawRectangleRounded(scaleRctangleFromCenter(rect, lerp(0.3, 1, effect)), roundness, 8, blendedCol);
        iconRerder(btnIcons[i], scaleRctangleFromCenter(rect, sizes[i]*lerp(0.9, 0.95, effect)), blendColors(COLOR_TEXT_1, COLOR_PALETTE_1_P9, effect));
        //if (isButtonClicked(btn)) funcs[i]();
    }
}


void updateControlLineButtons(float lineHeight) {
    float offsetXY = 0.1*lineHeight;
    float offsetXY_2 = 0.4*lineHeight;
    Rectangle rect = {0.2*lineHeight, 0.2*lineHeight, 0.6*lineHeight, 0.6*lineHeight};
    buttonUpdateRectangle(openFileButton, rect);
    int target = (layoutButton)?1:-1;
    buttonUpdate(openFileButton, target);

    
    rect.x += 2*rect.x+rect.width;
    Button btns[] = {previousButton, playPauseButton, nextButton};
    int btnNum = sizeof(btns)/sizeof(Button);
    for (int i=0; i<btnNum; i++) {
        buttonUpdateRectangle(btns[i], rect);
        buttonUpdate(btns[i], -1);
        rect.x += offsetXY+rect.width;
    }


    rect.x += offsetXY_2-offsetXY;
    //rect.height = 0.75*lineHeight, rect.y=0.125*lineHeight;
    rect.width = 2*lineHeight;
    textboxUpdateRectangle(tempoTextbox, rect);
    textboxUpdate(tempoTextbox, -1);
    if (isTextboxFocused(tempoTextbox)) {
        textboxUpdateText(tempoTextbox);
    }
    if (isTextboxJustUnfocused(tempoTextbox)) {
        int tempo = textboxGetPositiveInt(tempoTextbox); int ntempo=tempo;
        if (tempo>0) ntempo = projectSetTempo(tempo);
        if (tempo<=0 || ntempo!=tempo) textboxLoadPositiveInt(tempoTextbox, projectGetTempo());
    }


    const char* projectTitle = isTextboxFocused(projectTitleTextbox)?textboxGetText(projectTitleTextbox):projectGetCurrentTitle();
    if (!projectTitle) projectTitle = DEFAULT_PROJECT_TITLE;
    

    Vector2 dims = textFontGetSize(GlobalFonts[0].font, projectTitle, 0.36*lineHeight, 0);
    rect = centerRectangle((Vector2){0.5*screenSize.x, 0.5*lineHeight}, (Vector2){floatMax(floatMax(150, 4.5*lineHeight), dims.x+30), 0.6*lineHeight});

    textboxUpdateRectangle(projectTitleTextbox, rect);
    textboxUpdate(projectTitleTextbox, -1);
    if (isTextboxFocused(projectTitleTextbox)) {
        textboxUpdateText(projectTitleTextbox);

        projectTitle = textboxGetText(projectTitleTextbox);
        dims = textFontGetSize(GlobalFonts[0].font, projectTitle, 0.35*lineHeight, 0);
        rect = centerRectangle((Vector2){0.5*screenSize.x, 0.5*lineHeight}, (Vector2){floatMax(floatMax(150, 4.5*lineHeight), dims.x+30), 0.6*lineHeight});
        textboxUpdateRectangle(projectTitleTextbox, rect);
    }
    if (isTextboxJustUnfocused(projectTitleTextbox)) projectSetCurrentTitle(textboxGetText(projectTitleTextbox));
    

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
    Color col={30, 32, 38, 255};
    float effect = buttonGetEffectValue(btn);
    Color blend1 = blendColors(col, COLOR_PALETTE_1_P1, effect);
    Rectangle rect = buttonGetRectangle(btn);
    DrawRectangleRounded(rect, buttonGetRoundness(btn), 8, blend1);
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
    DrawRectangleRoundedLinesEx(brect, roundness, 8, 2, COLOR_PALETTE_1_BACKGROUND_3);  //COLOR_TEXT_4
}


void renderVerticalSeperator(float x, float lineHeight) {
    Color col = COLOR_TEXT_4; col.a=50;
    DrawLineEx((Vector2){x, 0.2*lineHeight}, (Vector2){x, 0.8*lineHeight}, floatMax(1, screenSize.x*0.002), col);
}


void renderControlLine() {

    float lineHeight = floatMin(0.08*screenSize.y, 100);
    updateControlLineButtons(lineHeight);



    DrawRectangle(0, 0, (int)(screenSize.x+2), (int)(lineHeight), COLOR_CONTROL_LINE_BACKGROUND);

    renderProjectTextbox();

    float effect = buttonGetEffectValue(openFileButton);
    Color col1 = {20, 21, 23, 0}, col2={28, 30, 34, 255};
    Color blendedCol = blendColors(col1, col2, effect);

    Rectangle rect = buttonGetRectangle(openFileButton);
    DrawRectangleRounded(scaleRctangleFromCenter(rect, lerp(0.3, 1, effect)), buttonGetRoundness(openFileButton), 8, blendedCol);
    iconRerder(T_ICON_MENU, scaleRctangleFromCenter(rect, lerp(0.85, 1, effect)), blendColors(COLOR_TEXT_1, COLOR_PALETTE_1_P9, effect));
    if (isButtonClicked(openFileButton)) createBaseLayout(lineHeight); //openFileButtonAction();


    renderVerticalSeperator(2*rect.x+rect.width, lineHeight);
    renderVerticalSeperator(textboxGetRectangle(tempoTextbox).x-rect.x, lineHeight);

    renderPrevPlayPauseNext();
    renderTempoTextbox();
    
    
    float gradientHeight = 0.1*lineHeight;
    Color topCol = {120, 139, 179, 255}, bottomCol={160, 160, 160, 0};
    DrawRectangleGradientV(0, (int)lineHeight, (int)(screenSize.x+2), (int)gradientHeight, topCol, bottomCol);
    DrawLineEx((Vector2){0, lineHeight}, (Vector2){screenSize.x, lineHeight}, 2, COLOR_TEXT_4);

    if (layoutButton) updateBaseLayout(lineHeight);
    if (layoutButton) renderBaseLayout(lineHeight);
}