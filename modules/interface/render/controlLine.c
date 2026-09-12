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
#include <synth.h>
#include <export.h>
#include <threads.h>


#define DEFAULT_PROJECT_TITLE "Untitled Project (1)"
#define PROJECT_TITLE_PLACEHOLDER "Project Title"


float controlLineHeight=0, buttonList4x5ExampleSpacing=0, buttonListTSExampleSpacing=0;
Rectangle buttonList4x5ExampleRect={0,0,0,0}, buttonList1x4ExampleRect={0,0,0,0}, buttonList1x6ExampleRect={0,0,0,0}, buttonList4x4ExampleRect={0,0,0,0}, buttonList2x4ExampleRect={0,0,0,0};

Button openFileButton = NULL;
ButtonList layoutButton = NULL, tsignNumBList=NULL, tsignDenBList=NULL, projectLayout=NULL, exportLayout=NULL;
Textbox projectTitleTextbox=NULL, tempoTextbox=NULL;
Button previousButton=NULL, playPauseButton=NULL, nextButton=NULL, loopButton=NULL, tsignatureNumButton=NULL, tsignatureDenButton=NULL;
Rectangle timeRect={0,0,0,0};



float lineHeight = 0;


void controlLineInit() {
    Rectangle rect = {20, 20, 80, 30};
    openFileButton = buttonCreate(rect, 0.25);

    projectTitleTextbox = textboxCreate(rect, 0.25, T_IN_STRING, 40);
    textboxLoadText(projectTitleTextbox, projectGetCurrentTitle());

    tempoTextbox = textboxCreate(rect, 0.25, T_IN_POSITIVE_INTEGER, 4);
    textboxLoadText(tempoTextbox, "120");

    tsignatureNumButton = buttonCreate(rect, 0.25);
    tsignatureDenButton = buttonCreate(rect, 0.25);

    previousButton = buttonCreate(rect, 0.25);
    playPauseButton = buttonCreate(rect, 0.25);
    nextButton = buttonCreate(rect, 0.25);
    loopButton = buttonCreate(rect, 0.25);
}

void destroyBaseLayout();
void updateTSbuttonLists();

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
    if (loopButton) buttonFree(loopButton);
    loopButton=NULL;

    if (tsignatureNumButton) buttonFree(tsignatureNumButton);
    tsignatureNumButton=NULL;
    if (tsignatureDenButton) buttonFree(tsignatureDenButton);
    tsignatureDenButton=NULL;

    if (tsignNumBList) buttonListFree(tsignNumBList, 1);
    tsignNumBList=NULL;
    if (tsignDenBList) buttonListFree(tsignDenBList, 1);
    tsignDenBList=NULL;
}


void createTimeSignatureNumeratorLayout() {
    if (tsignNumBList) buttonListFree(tsignNumBList, 1);
    Rectangle rect = buttonGetRectangle(tsignatureNumButton);
    buttonList1x6ExampleRect.x = rect.x;
    tsignNumBList = buttonListCreate(buttonList1x6ExampleRect, 6, 0.35, buttonListTSExampleSpacing, 1);
}

void createTimeSignatureDenominatorLayout() {
    if (tsignDenBList) buttonListFree(tsignDenBList, 1);
    Rectangle rect = buttonGetRectangle(tsignatureDenButton);
    buttonList1x4ExampleRect.x = rect.x;
    tsignDenBList = buttonListCreate(buttonList1x4ExampleRect, 4, 0.35, buttonListTSExampleSpacing, 1);
}

void createProjectLayout() {
    if (!projectLayout) {
        Rectangle rect = buttonListGetRect(layoutButton);
        buttonList4x4ExampleRect.x = rect.x+rect.width+interfaceSpace1;
        projectLayout = buttonListCreate(buttonList4x4ExampleRect, 4, 0.18, buttonList4x5ExampleSpacing, 1);
    }
    buttonListAttachChildLayout(layoutButton, &projectLayout);
}

void createExportLayout() {
    if (!exportLayout) {
        Rectangle rect = buttonListGetRect(layoutButton);
        buttonList2x4ExampleRect.x = rect.x+rect.width+interfaceSpace1;
        exportLayout = buttonListCreate(buttonList2x4ExampleRect, 4, 0.18, buttonList4x5ExampleSpacing, 1);
    }
    buttonListAttachChildLayout(layoutButton, &exportLayout);
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
        Color c1=COLOR_TEXT_4; c1.a=(unsigned char)lerp(0, 200, effect);
        iconRerder(T_ICON_EDIT, scaleRctangleFromCenter((Rectangle){rect.x+rect.width-rect.height, rect.y, rect.height, rect.height}, 0.4), c1);
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

void updateCLTextboxes() {
    if (tempoTextbox) textboxLoadPositiveInt(tempoTextbox, projectGetTempo());
    if (projectTitleTextbox) textboxLoadText(projectTitleTextbox, projectGetCurrentTitle());
}

void renderTempoTextbox() {
    const char* tempoTxt = textboxGetText(tempoTextbox);
    int isFocused = isTextboxFocused(tempoTextbox);
    float effect = textboxGetEffectValue(tempoTextbox);
    float roundness = textboxGetRoundness(tempoTextbox);
    Rectangle rect = textboxGetRectangle(tempoTextbox);
    int cursorIdx = textboxGetCursorIdx(tempoTextbox);

    char* tempoTxtShown=NULL;
    if (!isFocused) tempoTxtShown=concatenateStrings(tempoTxt, " BPM");

    Vector2 tarPos = lerpVector2_vec((Vector2){rect.x, rect.y}, (Vector2){rect.x+rect.width, rect.y+rect.height}, (Vector2){0.5, 0.5});

    Color col2={28, 30, 37, 255};
    Color blend1 = blendColors(col2, COLOR_PALETTE_1_NUM_1, effect);
    
    DrawRectangleRounded(rect, roundness, 8, blend1);
    //if (isFocused) DrawRectangleRoundedLinesEx(rect, roundness, 8, lerp(0, floatMin(1+3*isFocused, 0.05*rect.height), effect), blend2);
    
    Color blend3 = blendColors(COLOR_PALETTE_1_P9, COLOR_TEXT_1, 1);
    renderFontStringAlign(GlobalFonts[0].font, isFocused?tempoTxt:tempoTxtShown, tarPos, (Vector2){0.5, 0.5}, rect.height*0.5, 0, blend3);

    if (!isFocused) free(tempoTxtShown);

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

void renderTime() {
    Color bg={28, 30, 37, 255};
    DrawRectangleRounded(timeRect, 0.25, 8, bg);
    double ctime = globalHandlerGetLineTime();
    if (ctime<=999.999) renderFontStringAlign(GlobalFonts[0].font, TextFormat("%.3lf", ctime), getRectangleCenter(timeRect), (Vector2){0.5, 0.5}, timeRect.height*0.5, 0, COLOR_TEXT_1);
    else renderFontStringAlign(GlobalFonts[0].font, TextFormat("%.2lf", ctime), getRectangleCenter(timeRect), (Vector2){0.5, 0.5}, timeRect.height*0.5, 0, COLOR_TEXT_1);
}

void renderTimeSignature() {
    struct time_signature tsign = globalHandlerGetTimeSignature();


    Rectangle rect = buttonGetRectangle(tsignatureNumButton);
    float roundness = buttonGetRoundness(tsignatureNumButton), effect = buttonGetEffectValue(tsignatureNumButton);
    Color col1 = {20, 21, 23, 0}, col2={28, 30, 34, 255};
    Color bgcol = blendColors(col1, col2, effect);
    Color fgcol = blendColors(COLOR_PALETTE_1_P9, COLOR_TEXT_1, effect);

    DrawRectangleRounded(rect, roundness, 4, bgcol);
    renderFontStringAlign(GlobalFonts[0].font, TextFormat("%d", tsign.numerator), getRectangleCenter(rect), (Vector2){0.5,0.5}, rect.width, 0, fgcol);

    Vector2 cntr = {rect.x, rect.y+0.5*rect.height};

    rect = buttonGetRectangle(tsignatureDenButton);
    roundness = buttonGetRoundness(tsignatureDenButton), effect=buttonGetEffectValue(tsignatureDenButton);
    bgcol = blendColors(col1, col2, effect);
    fgcol = blendColors(COLOR_PALETTE_1_P9, COLOR_TEXT_1, effect);

    DrawRectangleRounded(rect, roundness, 4, bgcol);
    renderFontStringAlign(GlobalFonts[0].font, TextFormat("%d", tsign.denominator), getRectangleCenter(rect), (Vector2){0.5,0.5}, rect.width, 0, fgcol);


    cntr.x = 0.5*(cntr.x+rect.x+rect.width);
    renderFontStringAlign(GlobalFonts[0].font, "/", cntr, (Vector2){0.5,0.5}, rect.width, 0, COLOR_TEXT_1);


}


void clickedOnPlayPause() {
    buttonSetCurrentEffect(playPauseButton, 1);
    if (globalHandlerIsPlaying()) {
        globalHandlerPause();
        synthPanic();
    }
    else globalHandlerPlay();
}

void renderPrevPlayPauseNext() {
    int isPlaying = globalHandlerIsPlaying(), isLoopEnabled = globalHandlerIsLoopEnabled();

    OnClickFunc funcs[] = {globalHandlerSetToPreviousMeasure, clickedOnPlayPause, globalHandlerSetToNextMeasure, isLoopEnabled?globalHandlerDisableLoop:globalHandlerEnableLoop};
    Button btns[] = {previousButton, playPauseButton, nextButton, loopButton};
    enum icon_title btnIcons[] = {T_ICON_PREVIOUS, isPlaying?T_ICON_PAUSE:T_ICON_PLAY, T_ICON_NEXT, T_ICON_LOOP};
    float sizes[] = {0.85, isPlaying?0.8:0.65, 0.85, 0.88};
    int btnNum = sizeof(btns)/sizeof(Button);

    Color col1 = {20, 21, 23, 0}, col2={28, 30, 34, 255};
    

    for (int i=0; i<btnNum; i++) {
        Button btn = btns[i];

        float effect = buttonGetEffectValue(btn);
        float roundness = buttonGetRoundness(btn);
        int enabled = isButtonEnabled(btn);

        Rectangle rect = buttonGetRectangle(btn);
        Color tmp1=col1, tmp2=col2;
        if (btn==loopButton && isLoopEnabled) tmp2=COLOR_THEME_DARK_1;
        if (!enabled) tmp1=(Color){16, 17, 19, 120};

        Color blendedCol = blendColors(tmp1, tmp2, effect);
        
        if (effect>1e-3) DrawRectangleRounded(scaleRctangleFromCenter(rect, lerp(0.3, 1, effect)), roundness, 8, blendedCol);
        iconRerder(btnIcons[i], scaleRctangleFromCenter(rect, sizes[i]*lerp(0.9, 0.95, effect)), blendColors(enabled?COLOR_TEXT_1:COLOR_TEXT_4, COLOR_PALETTE_1_P9, effect));
        if (isButtonClicked(btn) && funcs[i]) actionDefer(funcs[i]);
        else {
            if (btn==playPauseButton && (!UIisInTextInput() && IsKeyPressed(KEY_SPACE))) actionDefer(funcs[i]);
        }
    }
}


void updateControlLineButtons() {
    float offsetXY = 0.1*lineHeight;
    float offsetXY_2 = 0.4*lineHeight;
    interfaceSpace1 = offsetXY, interfaceSpace2 = offsetXY_2;
    Rectangle rect = {0.2*lineHeight, 0.2*lineHeight, 0.6*lineHeight, 0.6*lineHeight};
    buttonUpdateRectangle(openFileButton, rect);
    int target = (layoutButton)?1:-1;
    buttonUpdate(openFileButton, target);

    
    rect.x += 2*rect.x+rect.width;
    Button btns[] = {previousButton, playPauseButton, nextButton, loopButton};
    uint32_t timeline = globalHandlerGetLineTimestamp();
    int isPlaying = globalHandlerIsPlaying();
    int btnsAllowed[] = {timeline && !isPlaying, 1, !isPlaying, 1};

    int btnNum = sizeof(btns)/sizeof(Button);
    for (int i=0; i<btnNum; i++) {
        buttonUpdateRectangle(btns[i], rect);
        if (btnsAllowed[i]) buttonEnable(btns[i]);
        else buttonDisable(btns[i]);

        if (btns[i]==loopButton && globalHandlerIsLoopEnabled()) buttonUpdate(btns[i], 1);
        else buttonUpdate(btns[i], -1);
        rect.x += offsetXY+rect.width;
    }

    rect.x+=offsetXY_2-offsetXY;
    rect.width = floatMin(2*lineHeight, 0.08*screenSize.x);
    timeRect = rect;


    rect.x += offsetXY_2+rect.width;
    //rect.height = 0.75*lineHeight, rect.y=0.125*lineHeight;
    rect.width = floatMin(2*lineHeight, 0.1*screenSize.x);
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


    rect.x += offsetXY_2+rect.width;
    rect.width = 0.4*lineHeight;
    buttonUpdateRectangle(tsignatureNumButton, rect);
    buttonUpdate(tsignatureNumButton, tsignNumBList?1:-1);
    if (!tsignNumBList && isButtonClicked(tsignatureNumButton)) actionDefer(createTimeSignatureNumeratorLayout);


    rect.x += offsetXY+rect.width;
    buttonUpdateRectangle(tsignatureDenButton, rect);
    buttonUpdate(tsignatureDenButton, tsignDenBList?1:-1);
    if (!tsignDenBList && isButtonClicked(tsignatureDenButton)) actionDefer(createTimeSignatureDenominatorLayout);


    updateTSbuttonLists();



    const char* projectTitle = isTextboxFocused(projectTitleTextbox)?textboxGetText(projectTitleTextbox):projectGetCurrentTitle();
    if (!projectTitle) projectTitle = DEFAULT_PROJECT_TITLE;
    
    rect.x += offsetXY_2+rect.width;
    float endX=screenSize.x-interfaceSpace1;
    Vector2 dims = textFontGetSize(GlobalFonts[0].font, projectTitle, 0.36*lineHeight, 0);
    Vector2 centerX = {0.5*(rect.x+endX), 0.5*lineHeight}; float spacing=floatMax(60, 1.2*lineHeight);
    rect = centerRectangle(centerX, (Vector2){floatMin(floatMax(floatMax(150, 5*lineHeight), dims.x+spacing), 0.5*screenSize.x), 0.6*lineHeight});

    textboxUpdateRectangle(projectTitleTextbox, rect);
    textboxUpdate(projectTitleTextbox, -1);
    if (isTextboxFocused(projectTitleTextbox)) {
        textboxUpdateText(projectTitleTextbox);

        projectTitle = textboxGetText(projectTitleTextbox);
        dims = textFontGetSize(GlobalFonts[0].font, projectTitle, 0.36*lineHeight, 0);
        rect = centerRectangle(centerX, (Vector2){floatMin(floatMax(floatMax(150, 5*lineHeight), dims.x+spacing), 0.5*screenSize.x), 0.6*lineHeight});
        textboxUpdateRectangle(projectTitleTextbox, rect);
    }
    if (isTextboxJustUnfocused(projectTitleTextbox)) projectSetCurrentTitle(textboxGetText(projectTitleTextbox));
    
}



void createBaseLayout() {
    if (layoutButton) buttonListFree(layoutButton, 1);
    layoutButton = buttonListCreate(buttonList4x5ExampleRect, 5, 0.18, buttonList4x5ExampleSpacing, 1);
    if (!layoutButton) return;
    int btns = buttonListGetNum(layoutButton);
    for (int i=0; i<btns; i++) {
        buttonUpdateCursorOnHover(buttonListGetButtonAt(layoutButton, i), MOUSE_CURSOR_POINTING_HAND);
    }
}

void destroyProjectLayout() {
    if (projectLayout) {
        if (layoutButton && buttonListGetAttachedChild(layoutButton)==&projectLayout) buttonListAttachChildLayout(layoutButton, NULL);
        buttonListFree(projectLayout, 1);
    }
    projectLayout = NULL;
}

void destroyExportLayout() {
    if (exportLayout) {
        if (layoutButton && buttonListGetAttachedChild(layoutButton)==&exportLayout) buttonListAttachChildLayout(layoutButton, NULL);
        buttonListFree(exportLayout, 1);
    }
    exportLayout = NULL;
}

void destroyBaseLayout() {
    if (layoutButton) buttonListFree(layoutButton, 1);
    layoutButton = NULL;

    destroyProjectLayout();
    destroyExportLayout();
}



void destroyTSignNumBL() {
    if (!tsignNumBList) return;
    buttonListFree(tsignNumBList, 1);
    tsignNumBList = NULL;
}

void destroyTSignDenBL() {
    if (!tsignDenBList) return;
    buttonListFree(tsignDenBList, 1);
    tsignDenBList = NULL;
}

void updateBaseLayout() {
    if (projectLayout) {
        buttonList4x4ExampleRect.y = buttonList4x5ExampleRect.y;
        buttonList4x4ExampleRect.x = buttonList4x5ExampleRect.x+buttonList4x5ExampleRect.width+interfaceSpace1;

        buttonListUpdateRect(projectLayout, buttonList4x4ExampleRect);
        buttonListUpdateSpacing(projectLayout, buttonList4x5ExampleSpacing);

        buttonListUpdateJustList(projectLayout);
        char allowed[] = {1,1,(char)(projectHasSavedFilepath() && projectHasUnsavedChanges()),1};
        char effects[] = {-1,-1,-1,-1};
        buttonListUpdateButtonsEnDisabled(projectLayout, 4, allowed, effects);

        if (buttonListShouldDelete(projectLayout)) destroyProjectLayout();
    }

    if (exportLayout) {
        buttonList2x4ExampleRect.y = buttonList4x5ExampleRect.y+buttonList4x5ExampleRect.height-buttonList2x4ExampleRect.height;
        buttonList2x4ExampleRect.x = buttonList4x5ExampleRect.x+buttonList4x5ExampleRect.width+interfaceSpace1;

        buttonListUpdateRect(exportLayout, buttonList2x4ExampleRect);
        buttonListUpdateSpacing(exportLayout, buttonList4x5ExampleSpacing);
        buttonListUpdateJustList(exportLayout);

        char allowed[] = {1,1,0,1};
        char effects[] = {-1,-1,-1,-1};

        buttonListUpdateButtonsEnDisabled(exportLayout, 4, allowed, effects);

        if (buttonListShouldDelete(exportLayout)) destroyExportLayout();
    }

    if (layoutButton) {
        buttonListUpdateRect(layoutButton, buttonList4x5ExampleRect);
        buttonListUpdateSpacing(layoutButton, buttonList4x5ExampleSpacing);
        ButtonList* selected = buttonListGetAttachedChild(layoutButton);
        if (!selected || !(*selected)) selected = NULL;

        buttonListUpdateJustList(layoutButton);

        char allowed[] = {1,1,1,1,1};
        char effects[] = {(selected==&projectLayout)?1:-1,-1,-1,-1,(selected==&exportLayout)?1:-1};

        buttonListUpdateButtonsEnDisabled(layoutButton, 5, allowed, effects);

        if (buttonListShouldDelete(layoutButton)) destroyBaseLayout();
    }

    

}

void updateTSbuttonLists() {
    if (tsignNumBList) {
        Rectangle rect = buttonGetRectangle(tsignatureNumButton);
        buttonList1x6ExampleRect.x = rect.x;
        buttonListUpdateRect(tsignNumBList, buttonList1x6ExampleRect);
        buttonListUpdateSpacing(tsignNumBList, buttonList4x5ExampleSpacing);
        buttonListUpdate(tsignNumBList);
        if (buttonListShouldDelete(tsignNumBList)) destroyTSignNumBL();
    }

    if (tsignDenBList) {
        Rectangle rect = buttonGetRectangle(tsignatureDenButton);
        buttonList1x4ExampleRect.x = rect.x;
        buttonListUpdateRect(tsignDenBList, buttonList1x4ExampleRect);
        buttonListUpdateSpacing(tsignDenBList, buttonList4x5ExampleSpacing);
        buttonListUpdate(tsignDenBList);
        if (buttonListShouldDelete(tsignDenBList)) destroyTSignDenBL();
    }
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

void renderTSlayoutButton(Button btn, const char* text, float textSize, int selected) {
    if (!btn) return;
    Color col={30, 32, 38, 255};
    float effect = floatMax(0.5*buttonGetEffectValue(btn), selected);
    Color blend1 = blendColors(col, COLOR_PALETTE_1_P1, effect);
    Rectangle rect = buttonGetRectangle(btn);
    DrawRectangleRounded(rect, buttonGetRoundness(btn), 8, blend1);
    renderFontStringAlign(GlobalFonts[0].font, text, getRectangleCenter(rect), (Vector2){0.5,0.5}, textSize, 0, COLOR_TEXT_1);
}


void renderBaseLayoutButton(Button btn, const char* text, Vector2 textAlign, Vector2 textOffset, float textSize, enum icon_title icon) {
    if (!btn) return;
    
    float effect = buttonGetEffectValue(btn);
    int enabled = isButtonEnabled(btn);

    Color col1={30, 32, 38, 255}, col2={16,17,21,255}, col3=enabled?COLOR_TEXT_1:COLOR_TEXT_4;

    Color blend1 = blendColors(enabled?col1:col2, COLOR_PALETTE_1_P1, effect);
    Rectangle rect = buttonGetRectangle(btn);
    DrawRectangleRounded(rect, buttonGetRoundness(btn), 8, blend1);
    Vector2 tarPos = lerpVector2_vec((Vector2){rect.x, rect.y}, (Vector2){rect.x+rect.width, rect.y+rect.height}, textAlign);
    renderFontStringAlign(GlobalFonts[0].font, text, Vector2Add(tarPos, textOffset), textAlign, textSize, 0, col3);
    if (effect>1e-5) {
        Color tar = col3;
        tar.a = 0;
        Color blend3 = blendColors(tar, col3, effect);
        Rectangle targRect = {rect.x+(0.75+0.25*effect)*rect.width-0.7*rect.height, rect.y+0.3*rect.height, 0.4*rect.height, 0.4*rect.height};
        if (icon!=T_ICON_END) iconRerder(icon, targRect, blend3);
    }
}

void exportProject() {
    globalHandlerPause();
    synthPanic();

    const char* projectTitle = projectGetCurrentTitle();
    if (!projectTitle) projectTitle = DEFAULT_PROJECT_TITLE;

    char* title = stringToFileName(projectTitle, 30);
    char* conct = concatenateStrings(title, ".mlb");
    free(title);

    const char* path = tinyfd_saveFileDialog("Export MidiLab Project", conct, 1, (const char *[]){"*.mlb"}, "MidiLab Project");
    free(conct);

    if (path) {
        //printf("Export to: %s\n", path);
        int failed = exportProjectTo(path);
        if (!failed) destroyBaseLayout();
    }
}

void exportProjectToSavedFilepath() {
    if (!projectHasSavedFilepath() || !projectHasUnsavedChanges()) return;

    globalHandlerPause();
    synthPanic();
    exportProjectToFilepath();
    destroyBaseLayout();
}

void exportProjectByCtrlS() {
    if (!projectHasUnsavedChanges()) return;
    //globalHandlerPause();
    //synthPanic();
    
    if (projectHasSavedFilepath()) exportProjectToFilepath();
    else {
        const char* projectTitle = projectGetCurrentTitle();
        if (!projectTitle) projectTitle = DEFAULT_PROJECT_TITLE;

        char* title = stringToFileName(projectTitle, 30);
        char* conct = concatenateStrings(title, ".mlb");
        free(title);

        const char* path = tinyfd_saveFileDialog("Export MidiLab Project", conct, 1, (const char *[]){"*.mlb"}, "MidiLab Project");
        free(conct);

        if (path) exportProjectTo(path);
    }
}

static void importProject() {
    globalHandlerPause();
    synthPanic();

    const char *path = tinyfd_openFileDialog("Import MidiLab Project", "", 1, (const char *[]){"*.mlb"}, "MidiLab Project", 0);
    if (path) {
        int canReplace = projectCanSafelyReplaceContents();
        if (!canReplace) {
            int result = tinyfd_messageBox("Warning", "Are you sure you want to load another project?\nYour current project will be lost.", "yesno", "warning", 0);
            if (!result) return;
        }

        printf("Trying to open: %s\n", path);
        int failed = importProjectFrom(path);
        if (!failed) destroyBaseLayout();
    }
}

static void exportWave() {
    globalHandlerPause();
    synthPanic();

    const char* projectTitle = projectGetCurrentTitle();
    if (!projectTitle) projectTitle = DEFAULT_PROJECT_TITLE;

    char* title = stringToFileName(projectTitle, 30);
    char* conct = concatenateStrings(title, ".wav");
    free(title);

    const char* path = tinyfd_saveFileDialog("Export MidiLab Project As .WAV", conct, 1, (const char *[]){"*.wav"}, "WAVE Format");
    free(conct);

    if (path) {
        int failed = threadRequestExportWave(path);
        if (!failed) destroyBaseLayout();
    }
}

static void exportMidi() {
    globalHandlerPause();
    synthPanic();

    const char* projectTitle = projectGetCurrentTitle();
    if (!projectTitle) projectTitle = DEFAULT_PROJECT_TITLE;

    char* title = stringToFileName(projectTitle, 30);
    char* conct = concatenateStrings(title, ".mid");
    free(title);

    const char* path = tinyfd_saveFileDialog("Export MidiLab Project As .MID", conct, 1, (const char *[]){"*.mid"}, "MIDI Format");
    free(conct);

    if (path) {
        int failed = exportProjectAsMidi(path);
        if (!failed) destroyBaseLayout();
    }
}

static void _exportAll() {
    globalHandlerPause();
    synthPanic();
    const char* path = tinyfd_selectFolderDialog("Export Everything In Directory", "");
    if (path) {
        int failed = threadRequestExportAll(path);
        if (!failed) destroyBaseLayout();
    }
}

void deferNewProject() {
    globalHandlerPause();
    synthPanic();

    int result = 1;
    int canReplace = projectCanSafelyReplaceContents();
    if (!canReplace) {
        result = tinyfd_messageBox("Warning", "Are you sure you want to create a new project?\nYour current project will be lost.", "yesno", "warning", 0);
    }

    if (result==1) newProject();
    destroyBaseLayout();
}

void renderBaseLayout() {
    if (!layoutButton) return;
    Rectangle brect = buttonListGetRect(layoutButton);
    Color col1 = {20, 21, 23, 255};
    float roundness = buttonListGetRoundness(layoutButton);
    DrawRectangleRoundedLinesEx(brect, roundness, 8, 8, (Color){2, 2, 2, 100});
    DrawRectangleRounded(brect, roundness, 8, col1);
    const char* texts[] = {"Project", "Edit", "View", "Settings", "Export"};
    OnClickFunc actions[] = {createProjectLayout, NULL, NULL, NULL, createExportLayout};
    int num = buttonListGetNum(layoutButton);
    for (int i=0; i<num; i++) {
        Button btn = buttonListGetButtonAt(layoutButton, i);
        renderBaseLayoutButton(btn, texts[i], (Vector2){0, 0.5}, (Vector2){10, 0}, brect.height*0.08, T_ICON_RIGHT);
        if (actions[i] && isButtonClicked(btn)) {
            actionDefer(actions[i]);
        }
    }
    DrawRectangleRoundedLinesEx(brect, roundness, 8, 2, COLOR_PALETTE_1_BACKGROUND_3);  //COLOR_TEXT_4


    if (projectLayout) {
        Rectangle brect = buttonListGetRect(projectLayout);
        Color col1 = {20, 21, 23, 255};
        float roundness = buttonListGetRoundness(projectLayout);
        DrawRectangleRoundedLinesEx(brect, roundness, 8, 8, (Color){2, 2, 2, 100});
        DrawRectangleRounded(brect, roundness, 8, col1);
        const char* texts[] = {"New Project", "Load Project", "Save", "Save As"};
        OnClickFunc actions[] = {deferNewProject, importProject, exportProjectToSavedFilepath, exportProject};
        int num = buttonListGetNum(projectLayout);
        for (int i=0; i<num; i++) {
            Button btn = buttonListGetButtonAt(projectLayout, i);
            renderBaseLayoutButton(btn, texts[i], (Vector2){0, 0.5}, (Vector2){10, 0}, brect.height*0.11, T_ICON_END);
            if (actions[i] && isButtonClicked(btn)) actionDefer(actions[i]);
        }
        DrawRectangleRoundedLinesEx(brect, roundness, 8, 2, COLOR_PALETTE_1_BACKGROUND_3);
    }

    if (exportLayout) {
        Rectangle brect = buttonListGetRect(exportLayout);
        Color col1 = {20, 21, 23, 255};
        float roundness = buttonListGetRoundness(exportLayout);
        DrawRectangleRoundedLinesEx(brect, roundness, 8, 8, (Color){2, 2, 2, 100});
        DrawRectangleRounded(brect, roundness, 8, col1);
        const char* texts[] = {"MIDI", "WAV", "MP3", "ALL"};
        OnClickFunc actions[] = {exportMidi, exportWave, NULL, _exportAll};
        int num = buttonListGetNum(exportLayout);
        for (int i=0; i<num; i++) {
            Button btn = buttonListGetButtonAt(exportLayout, i);
            renderBaseLayoutButton(btn, texts[i], (Vector2){0, 0.5}, (Vector2){10, 0}, brect.height*0.11, T_ICON_END);
            if (actions[i] && isButtonClicked(btn)) actionDefer(actions[i]);
        }
        DrawRectangleRoundedLinesEx(brect, roundness, 8, 2, COLOR_PALETTE_1_BACKGROUND_3);
    }
}

void renderNumBlist() {
    //printf("%p\n", (void*)tsignNumBList);
    if (!tsignNumBList) return;
    struct time_signature tsign = globalHandlerGetTimeSignature();
    Rectangle brect = buttonListGetRect(tsignNumBList);
    Color col1 = {20, 21, 23, 255};
    float roundness = buttonListGetRoundness(tsignNumBList);
    DrawRectangleRoundedLinesEx(brect, roundness, 8, 8, (Color){2, 2, 2, 100});
    DrawRectangleRounded(brect, roundness, 8, col1);
    const char* texts[] = {"1", "2", "3", "4", "5", "6"};
    int num = buttonListGetNum(tsignNumBList);
    for (int i=0; i<num; i++) {
        Button btn = buttonListGetButtonAt(tsignNumBList, i);
        renderTSlayoutButton(btn, texts[i], brect.width*0.4, tsign.numerator==i+1);
        if (tsign.numerator!=i+1 && isButtonClicked(btn)) globalHandlerSetTimeSignature((struct time_signature){i+1, tsign.denominator});
    }
    DrawRectangleRoundedLinesEx(brect, roundness, 8, 2, COLOR_PALETTE_1_BACKGROUND_3);  //COLOR_TEXT_4
}

void renderDenBlist() {
    if (!tsignDenBList) return;
    struct time_signature tsign = globalHandlerGetTimeSignature();
    Rectangle brect = buttonListGetRect(tsignDenBList);
    Color col1 = {20, 21, 23, 255};
    float roundness = buttonListGetRoundness(tsignDenBList);
    DrawRectangleRoundedLinesEx(brect, roundness, 8, 8, (Color){2, 2, 2, 100});
    DrawRectangleRounded(brect, roundness, 8, col1);
    const char* texts[] = {"1", "2", "4", "8"};
    int num = buttonListGetNum(tsignDenBList);
    for (int i=0; i<num; i++) {
        Button btn = buttonListGetButtonAt(tsignDenBList, i);
        renderTSlayoutButton(btn, texts[i], brect.width*0.4, tsign.denominator==(1<<i));
        if (tsign.denominator!=(1<<i) && isButtonClicked(btn)) globalHandlerSetTimeSignature((struct time_signature){tsign.numerator, (1<<i)});
    }
    DrawRectangleRoundedLinesEx(brect, roundness, 8, 2, COLOR_PALETTE_1_BACKGROUND_3);  //COLOR_TEXT_4
}


void renderVerticalSeperator(float x) {
    Color col = COLOR_TEXT_4; col.a=50;
    DrawLineEx((Vector2){x, 0.2*lineHeight}, (Vector2){x, 0.8*lineHeight}, floatMax(1, screenSize.x*0.002), col);
}


void order1PrecomputeControlLine() {
    float tl = screenSize.y;
    if (screenSize.y/screenSize.x>0.666) tl = screenSize.x*0.666;
    controlLineHeight = (lineHeight = floatMin(0.08*tl, 100));

    buttonList4x5ExampleRect = (Rectangle){0.2*lineHeight, 0.9*lineHeight, floatMax(2*lineHeight, 120), floatMax(3*lineHeight, 180)};
    buttonList4x5ExampleSpacing = floatMax(0.08*lineHeight, 4.8);

    buttonList1x6ExampleRect = (Rectangle){0, 0.9*lineHeight, floatMax(0.8*lineHeight, 30), floatMax(3.6*lineHeight, 216)};
    buttonList1x4ExampleRect = (Rectangle){0, 0.9*lineHeight, floatMax(0.8*lineHeight, 30), floatMax(2.4*lineHeight, 144)};
    buttonListTSExampleSpacing = floatMax(0.032*lineHeight, 4.8);

    buttonList4x4ExampleRect = (Rectangle){0, 0, floatMax(2.1*lineHeight, 120), buttonList4x5ExampleRect.height*0.8};
    buttonList2x4ExampleRect = (Rectangle){0, 0, floatMax(1.3*lineHeight, 60), buttonList4x5ExampleRect.height*0.8};

    updateControlLineButtons();

    if (!layoutButton && isButtonClicked(openFileButton)) actionDefer(createBaseLayout);
    if (layoutButton) updateBaseLayout();
}

void order2PrecomputeControlLine() {

}

static void renderSaveState() {
    int unsaved = projectHasUnsavedChanges();
    if (!unsaved) return;

    Rectangle trect = textboxGetRectangle(projectTitleTextbox);
    float effect = textboxGetEffectValue(projectTitleTextbox);
    DrawCircleV((Vector2){trect.x+trect.width-trect.height*0.5, trect.y+0.5*trect.height}, floatMax(4, 0.07*lineHeight), (Color){180,180,180,(unsigned char)(255*(1.0-effect))});
    //DrawCircleV((Vector2){screenSize.x-lineHeight*0.5, lineHeight*0.5}, floatMax(4, 0.03*lineHeight), (Color){130,130,130,255});
}

void renderControlLine() {
    
    DrawRectangle(0, 0, (int)(screenSize.x+2), (int)(lineHeight), COLOR_CONTROL_LINE_BACKGROUND);

    renderProjectTextbox();

    float effect = buttonGetEffectValue(openFileButton);
    Color col1 = {20, 21, 23, 0}, col2={28, 30, 34, 255};
    Color blendedCol = blendColors(col1, col2, effect);

    Rectangle rect = buttonGetRectangle(openFileButton);
    DrawRectangleRounded(scaleRctangleFromCenter(rect, lerp(0.3, 1, effect)), buttonGetRoundness(openFileButton), 8, blendedCol);
    iconRerder(T_ICON_MENU, scaleRctangleFromCenter(rect, lerp(0.85, 1, effect)), blendColors(COLOR_TEXT_1, COLOR_PALETTE_1_P9, effect));


    renderVerticalSeperator(2*rect.x+rect.width);
    renderVerticalSeperator(timeRect.x-rect.x);
    renderVerticalSeperator(textboxGetRectangle(tempoTextbox).x-rect.x);
    renderVerticalSeperator(buttonGetRectangle(tsignatureNumButton).x-rect.x);
    Rectangle dnmRect = buttonGetRectangle(tsignatureDenButton);
    renderVerticalSeperator(dnmRect.x+dnmRect.width+interfaceSpace2-rect.x);
    

    renderPrevPlayPauseNext();
    renderTempoTextbox();
    renderTimeSignature();
    renderTime();
    
    
    //float gradientHeight = 0.1*lineHeight;
    //Color topCol = {120, 139, 179, 255}, bottomCol={160, 160, 160, 0};
    //DrawRectangleGradientV(0, (int)lineHeight, (int)(screenSize.x+2), (int)gradientHeight, topCol, bottomCol);
    DrawLineEx((Vector2){0, lineHeight-1}, (Vector2){screenSize.x, lineHeight-1}, 2, COLOR_TEXT_4);

    if (tsignNumBList) renderNumBlist();
    if (tsignDenBList) renderDenBlist();
    if (layoutButton) renderBaseLayout();

    renderSaveState();
}