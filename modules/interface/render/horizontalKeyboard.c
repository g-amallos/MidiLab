#include <interface.h>
#include <backend.h>
#include <synth.h>
#include <handler.h>
#include <raymath.h>
#include <colors.h>
#include <utils.h>
#include <ui.h>
#include <stdlib.h>
#include <stdio.h>
#include <images.h>


#define keyboardRangeShown (84-36+1)

float horizontalKeyboardHeight=0;

static Button changeProgramButton=NULL;
static Slider panningSlider=NULL, volumeSlider=NULL;
static Button keyButtons[keyboardRangeShown] = {NULL};
static int keyboardShownStartOffset=36, keyboardInputStartOffset=12;

static float space=0, wkWidth=0, hkHeight=0, hkTopPadding=0, offsetY=0, posY=0, blackKeyRelativeHeight=0.55, blackKeyRelativeWidth=0.65, upperRectHeight=0;
static int keyStart=0, keyEnd=0, numOfWhiteKeys=0;
static Rectangle rect={0,0,0,0}, leftRect={0,0,0,0}, rightRect={0,0,0,0};




void horizontalKeyboardInit() {
    changeProgramButton = buttonCreate((Rectangle){20,20,20,20}, 0.25);
    buttonSetEffectSpeed(changeProgramButton, 0.12);

    panningSlider=sliderCreate((Rectangle){20,20,20,20},1);
    sliderUpdateCursorOnHover(panningSlider, MOUSE_CURSOR_RESIZE_NS);
    volumeSlider=sliderCreate((Rectangle){20,20,20,20},1);
    sliderUpdateCursorOnHover(volumeSlider, MOUSE_CURSOR_RESIZE_NS);

    for (int i=0; i<keyboardRangeShown; i++) {
        keyButtons[i]=buttonCreate((Rectangle){20,20,20,20},0);
        buttonSetEffectSpeed(keyButtons[i], 0.35);
    }
}


void horizontalKeyboardClose() {
    if (changeProgramButton) buttonFree(changeProgramButton);
    changeProgramButton=NULL;

    if (panningSlider) sliderFree(panningSlider);
    panningSlider=NULL;

    if (volumeSlider) sliderFree(volumeSlider);
    volumeSlider=NULL;

    for (int i=0; i<keyboardRangeShown; i++) {
        if (keyButtons[i]) buttonFree(keyButtons[i]);
        keyButtons[i] = NULL;
    }
}


int getWhiteKeysNum(int keyStart, int keyEnd) {
    if (keyEnd<keyStart) return 0;
    char keyTypes[12] = {0,1,0,1,0,0,1,0,1,0,1,0};
    if (keyTypes[keyStart%12] || keyTypes[keyEnd%12]) return 0;
    int ret=0;
    for (int i=keyStart; i<=keyEnd; i++) {
        ret += !(keyTypes[i%12]);
    }
    return ret;
}

int isKeyInInputKeyboard(int i) {
    int keyInRangeWhite=12;
    if (i<keyboardInputStartOffset+keyboardShownStartOffset) return 0;
    if (i-keyboardInputStartOffset-keyboardShownStartOffset<=12) return 1;

    char types[12]={0,1,0,1,0,0,1,0,1,0,1,0};
    int keyboardInputEnd=keyboardInputStartOffset+keyboardShownStartOffset+12, tkeys=7;
    while (tkeys<keyInRangeWhite) {
        if (!(types[keyboardInputEnd%12])) tkeys++;
        keyboardInputEnd++;
    }
    return (i<keyboardInputEnd);
}


int getKeyFromChar(char c) {
    if (c >= 'a' && c <= 'z') c -= 32; 
    
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        return (int)c;
    }
    switch (c) {
        case ';': return KEY_SEMICOLON;
        case '\'': return KEY_APOSTROPHE;
        case '\\': return KEY_BACKSLASH;
        case '[': return KEY_LEFT_BRACKET;
        case ']': return KEY_RIGHT_BRACKET;
        default: return KEY_NULL;
    }
}


void temporarySolutionChangeInstrument() {
    int sel = globalHandlerGetSelectedTrack();
    if (sel<0) return;
    Track track = trackGetAtIdx(sel);
    int program = trackGetProgram(track);
    synthPanic();
    trackSetProgram(track, (uint8_t)((program+1)%129));
}

void resetVisuallyTheKeys() {
    for (int i=0; i<keyboardRangeShown; i++) {
        if (keyButtons[i]) buttonSetCurrentEffect(keyButtons[i], 0);
    }
}

void moveVirtualKeyboardIfNeeded() {
    int d1=IsKeyPressed(KEY_RIGHT)-IsKeyPressed(KEY_LEFT);
    int d2=IsKeyPressed(KEY_KP_ADD)-IsKeyPressed(KEY_KP_SUBTRACT);
    int cur = keyboardShownStartOffset+keyboardInputStartOffset;
    int mod=cur%12;

    if (d1||d2) synthPanic();
    if (d2) resetVisuallyTheKeys();

    if (d1>0) {
        if (mod==4 || mod==11) keyboardInputStartOffset+=1; else keyboardInputStartOffset+=2;
    } else if (d1<0) {
        if (mod==0 || mod==5) keyboardInputStartOffset-=1; else keyboardInputStartOffset-=2;
    }

    if (d2>0 && keyboardShownStartOffset+keyboardRangeShown<120) keyboardShownStartOffset+=12;
    else if (d2<0 && keyboardShownStartOffset>0) keyboardShownStartOffset-=12;

    if (keyboardInputStartOffset<0) keyboardInputStartOffset=0;
    if (keyboardInputStartOffset+19>=keyboardShownStartOffset+keyboardRangeShown) keyboardInputStartOffset=keyboardShownStartOffset+keyboardRangeShown-20;
}



float getVerticalDifferenceScroll() {
    return (globalMouseHandler.pos.y-globalMouseHandler.clickPos.y)/screenSize.y;
}

float getVerticalDy() {
    return globalMouseHandler.dpos.y/screenSize.y;
}


void precalculateSizesHorizontalKeyboard() {
    space=0.75*interfaceSpace1;
    keyStart=keyboardShownStartOffset;
    keyEnd=keyStart+keyboardRangeShown-1;
    numOfWhiteKeys = getWhiteKeysNum(keyStart, keyEnd);


    upperRectHeight = floatMax(120, 0.15*screenSize.y);
    leftRect = (Rectangle){interfaceSpace1+space, screenSize.y-bottomHalfUsefulHeight+interfaceSpace1+space, 0.3*screenSize.x, upperRectHeight};
    rightRect = (Rectangle){0.7*screenSize.x-interfaceSpace1-space, screenSize.y-bottomHalfUsefulHeight+interfaceSpace1+space, 0.3*screenSize.x, upperRectHeight};

    wkWidth = (screenSize.x-2*interfaceSpace1-(numOfWhiteKeys+1)*space)/numOfWhiteKeys;
    hkHeight=floatMin(wkWidth*4+2*space, floatMax(250, bottomHalfUsefulHeight));
    hkTopPadding = 150;

    offsetY = floatMax(hkTopPadding, bottomHalfUsefulHeight-hkHeight-interfaceSpace1);
    posY = floatMax(leftRect.y+leftRect.height+interfaceSpace1+space, screenSize.y-hkHeight-interfaceSpace1);

    rect = (Rectangle){interfaceSpace1, posY, screenSize.x-2*interfaceSpace1, hkHeight};

    horizontalKeyboardHeight=hkHeight+leftRect.height+3*(interfaceSpace1+space);
}

void precalculateJustHorizontalKeyboard() {
    // Must have already ran `precalculateSizesHorizontalKeyboard()`


    Track curTrack = trackGetAtIdx(globalHandlerGetSelectedTrack());

    if (panningSlider) {
        float tsize = 0.55*floatMin(leftRect.height, leftRect.width*0.5);
        Rectangle trect = centerRectangle((Vector2){leftRect.x+leftRect.width*0.25, leftRect.y+leftRect.height*0.5+0.15*tsize}, (Vector2){tsize, tsize});
        sliderUpdateRectangle(panningSlider, trect);
        sliderUpdate(panningSlider, -1);

        if (isSliderDragged(panningSlider)) {
            float dy = -4*getVerticalDy();
            float nval = floatClip(sliderGetSlideValue(panningSlider)+dy, 0, 1);
            sliderUpdateSlideValue(panningSlider, nval);
            trackSetPanning(curTrack, nval);
        } else sliderUpdateSlideValue(panningSlider, trackGetPanning(curTrack));
    }

    if (volumeSlider) {
        float tsize = 0.55*floatMin(leftRect.height, leftRect.width*0.5);
        Rectangle trect = centerRectangle((Vector2){leftRect.x+leftRect.width*0.75, leftRect.y+leftRect.height*0.5+0.15*tsize}, (Vector2){tsize, tsize});
        sliderUpdateRectangle(volumeSlider, trect);
        sliderUpdate(volumeSlider, -1);

        if (isSliderDragged(volumeSlider)) {
            float dy = -4*getVerticalDy();
            float nval = floatClip(sliderGetSlideValue(volumeSlider)+dy, 0, 1);
            sliderUpdateSlideValue(volumeSlider, nval);
            trackSetVelocity(curTrack, nval);
        } else sliderUpdateSlideValue(volumeSlider, trackGetVelocity(curTrack));
    }



    Rectangle brect={0,0,0,0};

    if (changeProgramButton) {
        float th = floatMax(40, 0.06*screenSize.y);
        brect = centerRectangle((Vector2){screenSize.x*0.5, leftRect.y+leftRect.height-0.5*th}, (Vector2){floatMax(300, 0.22*screenSize.x), th});
        buttonUpdateRectangle(changeProgramButton, brect);
        buttonUpdate(changeProgramButton, -1);

        if (isButtonClicked(changeProgramButton)) actionDefer(createInstrumentPicker);
    }

    



    
    
}




void renderJustHorizontalKeyboard() {
    int clickingAllowed = !UIexistsFrontLayoutOverlay();
    int keyboardInputEnabled = !UIisInTextInput() && clickingAllowed;
    if (keyboardInputEnabled) moveVirtualKeyboardIfNeeded();

    keyStart=keyboardShownStartOffset;
    keyEnd=keyStart+keyboardRangeShown-1;



    Color keyboardColor = {19, 20, 25, 255};
    DrawRectangleRounded(rect, getRoundnessForRoundedRectangle(rect, 2*space), 8, keyboardColor);



    
    float bkWidth=blackKeyRelativeWidth*wkWidth, wkHeight=rect.height-2*space;
    float wkSkY = rect.y+2*space+blackKeyRelativeHeight*wkHeight; float wkSkH=rect.height-space+rect.y-wkSkY, bkHeight=blackKeyRelativeHeight*wkHeight;
    float wkTkY=rect.y+space, wkTkW=wkWidth-2*space; float wkTkH=wkSkY-wkTkY+2*space, txtSize=0.5*wkWidth;

    int keyRTypes[12] = {0,3,1,3,2,0,3,1,3,1,3,2};
    char keyTypes[12] = {0,1,0,1,0,0,1,0,1,0,1,0};
    char txt1[12]={'C','C','D','D','E','F','F','G','G','A','A','B'};

    float pXOffset[3] = {0, 0.5*bkWidth, 0.5*bkWidth};
    float pWOffset[3] = {wkWidth-0.5*bkWidth-space, wkWidth-bkWidth-space, wkWidth-0.5*bkWidth};

    Color keyWhite = {200, 200, 200, 255}, keyBlack={12, 12, 12, 255};
    Color keyColors[8] = {COLOR_KEYBOARD_H_KEY_N_SELECTED_WHITE, COLOR_KEYBOARD_H_KEY_N_SELECTED_WHITE_HOVERED, COLOR_KEYBOARD_H_KEY_N_SELECTED_BLACK, COLOR_KEYBOARD_H_KEY_N_SELECTED_BLACK_HOVERED, COLOR_KEYBOARD_H_KEY_SELECTED_WHITE, COLOR_KEYBOARD_H_KEY_SELECTED_WHITE_HOVERED, COLOR_KEYBOARD_H_KEY_SELECTED_BLACK, COLOR_KEYBOARD_H_KEY_SELECTED_BLACK_HOVERED};
    
    char keyWhiteInputChars[] = {'A','S','D','F','G','H','J','K','L',';','\'','\\'};
    char keyBlackInputChars[] = {'W','E','R','T','Y','U','I','O','P','[',']'};

    

    //if (keyboardInputEnabled && IsKeyPressed(KEY_ENTER)) actionDefer(temporarySolutionChangeInstrument);

    for (int type=0; type<=1; type++) {
        float posX=rect.x+space+type*(wkWidth-0.5*bkWidth-space);
        int inputIdxChar = (type==1 && (((keyStart+keyboardInputStartOffset)%12)==4 || ((keyStart+keyboardInputStartOffset)%12)==11));
        for (int i=keyStart; i<=keyEnd; i++) {
            int mod = i%12;
            if (keyTypes[mod]!=type) continue;
             
            char txt[3]={txt1[mod], (type)?'#':(mod?0:('0'+(i+1)/12)), 0};
            Button btn = keyButtons[i-keyStart];
            int isInInput = isKeyInInputKeyboard(i), effect=0, clicked=0, released=0, hover=0;

            if (isInInput&&keyboardInputEnabled) {
                int rkey = getKeyFromChar(type?keyBlackInputChars[inputIdxChar]:keyWhiteInputChars[inputIdxChar]);
                clicked=IsKeyPressed(rkey);
                effect=IsKeyDown(rkey);
                released=IsKeyReleased(rkey);
            }

            Color col = blendColors(keyColors[4*isInInput+2*type], keyColors[4*isInInput+2*type+1], buttonGetEffectValue(btn));
            

            if (type==0) {
                if ((i==keyStart && mod==11) || (i==keyEnd && mod==0)) {
                    Rectangle krect = {posX, rect.y+space, wkWidth, wkHeight};
                    float roundness=getRoundnessForRoundedRectangle(krect, space);
                    DrawRectangleRounded(krect, roundness, 5, col);

                    hover |= (clickingAllowed && checkCollisionPointRoundedRect(globalMouseHandler.pos, krect, roundness));
                    effect |= hover;
                    buttonUpdateCustomHoverEffect(btn, hover, effect);

                } else {
                    Rectangle krect = {posX, wkSkY, wkWidth, wkSkH};
                    float roundness=getRoundnessForRoundedRectangle(krect, space);
                    hover = (clickingAllowed && checkCollisionPointRoundedRect(globalMouseHandler.pos, krect, roundness));
                    DrawRectangleRounded(krect, roundness, 5, col);
                    

                    krect = (Rectangle){posX+keyRTypes[mod]*space, wkTkY, wkTkW, wkTkH};
                    roundness=getRoundnessForRoundedRectangle(krect, space);
                    hover |= (clickingAllowed && checkCollisionPointRoundedRect(globalMouseHandler.pos, (Rectangle){posX+pXOffset[keyRTypes[mod]], wkTkY, pWOffset[keyRTypes[mod]], wkTkH}, 0));
                    effect |= hover;
                    DrawRectangleRounded(krect, roundness, 5, col);
                    buttonUpdateCustomHoverEffect(btn, hover, effect);
                }

                if (isInInput && keyboardInputEnabled) {
                    char tmpTxt[] = {keyWhiteInputChars[inputIdxChar++],0};
                    renderFontStringAlign(GlobalFonts[0].font, tmpTxt, (Vector2){posX+0.5*wkWidth, rect.y+(0.75*wkHeight+0.25*bkHeight)-interfaceSpace1}, (Vector2){0.5, 1}, 0.8*txtSize, 0, keyBlack);
                }

                if (!mod) renderFontStringAlign(GlobalFonts[0].font, txt, (Vector2){posX+0.5*wkWidth, rect.y+wkHeight-interfaceSpace1}, (Vector2){0.5, 1}, txtSize, 0, keyBlack);
                //renderFontStringAlign(GlobalFonts[0].font, txt, (Vector2){posX+0.5*wkWidth, rect.y+(0.75*wkHeight+0.25*bkHeight)-interfaceSpace1}, (Vector2){0.5, 1}, txtSize, 0, keyBlack);

                posX+=wkWidth+space;
            } else {
                Rectangle krect = {posX, rect.y, bkWidth+2*space, bkHeight+2*space};
                DrawRectangleRounded(krect, getRoundnessForRoundedRectangle(krect, space), 5, keyboardColor);

                krect = (Rectangle){posX+space, rect.y, bkWidth, bkHeight+space};
                float roundness=getRoundnessForRoundedRectangle(krect, space);
                DrawRectangleRounded(krect, roundness, 5, col);

                hover |= (clickingAllowed && checkCollisionPointRoundedRect(globalMouseHandler.pos, krect, roundness));
                effect |= hover;
                buttonUpdateCustomHoverEffect(btn, hover, effect);

                //renderFontStringAlign(GlobalFonts[0].font, txt, (Vector2){posX+0.5*bkWidth+space, rect.y+bkHeight-interfaceSpace1}, (Vector2){0.5, 1}, txtSize, 0, keyWhite);

                if (isInInput && keyboardInputEnabled) {
                    char tmpTxt[] = {keyBlackInputChars[inputIdxChar++],0};
                    if (mod==3 || mod==10) inputIdxChar++;
                    renderFontStringAlign(GlobalFonts[0].font, tmpTxt, (Vector2){posX+0.5*bkWidth+space, rect.y+bkHeight-interfaceSpace1}, (Vector2){0.5, 1}, 0.8*txtSize, 0, keyWhite);
                }

                if (mod==1 || mod==6 || mod==8) posX+=wkWidth+space;
                else posX+=2*(wkWidth+space);
            }

            if (clicked || isButtonClicked(btn)) globalHandlerUpdateKeyAndPlaySynth(i, (uint8_t)(127*trackGetVelocity(trackGetAtIdx(globalHandlerGetSelectedTrack()))));
            if (released || isButtonReleased(btn)) globalHandlerUpdateKeyAndPlaySynth(i, 0);
        }
    }
    DrawRectangleRec((Rectangle){rect.x+space, rect.y, rect.width-2*space, space}, keyboardColor);
}


void renderChangeInstrumentButton() {
    if (changeProgramButton) {
        Rectangle brect = buttonGetRectangle(changeProgramButton);
        float roundness = buttonGetRoundness(changeProgramButton);
        float effect = buttonGetEffectValue(changeProgramButton);
        Track track = trackGetAtIdx(globalHandlerGetSelectedTrack());
        int programIdx = trackGetProgram(track);
        const char* instName = midiGetProgramName(programIdx);
        enum midi_program_type mt = midiGetProgramType(programIdx);
        Color typeTheme = programTypeColors[mt];
        DrawRectangleRounded(brect, roundness, 8, blendColors((Color){20, 20, 20, 255}, typeTheme, 0.1));//lerp(0.1, 0.6, effect)));
        
        DrawRectangleRounded(scaleRctangleFromCenter(brect, effect), roundness, 8, blendColors((Color){20, 20, 20, 60}, typeTheme, lerp(0.05, 0.5, effect)));
        renderFontStringAlign(GlobalFonts[0].font, instName, getRectangleCenter(brect), (Vector2){0.5,0.5}, 0.08*brect.width, 0, (Color){200, 200, 200, 255});
        //float thickness=lerp(4, 20, effect);
        //Rectangle irect = (Rectangle){brect.x+thickness*0.5, brect.y+0.5*thickness, brect.width-thickness, brect.height-thickness};
        //
        //DrawRectangleRoundedLinesEx(irect, getRoundnessForRoundedRectangle(irect, getRadiusForRoundedRectangle(rect, roundness)), 8, thickness, blendColors((Color){20, 20, 20, 255}, typeTheme, lerp(0.3, 0.6, effect)));
        
        DrawRectangleRoundedLinesEx(brect, roundness, 8, 2, (Color){200, 200, 200, 255});
        renderFontStringAlign(GlobalFonts[0].font, midiGetProgramTypeString(programIdx), (Vector2){brect.x+brect.width*0.5, brect.y-brect.height*0.5}, (Vector2){0.5,0.5}, 0.08*brect.width, 0, (Color){200, 200, 200, 255});
    }
}


void renderSoundPanelEffects() {
    Color col= {27,28,30,255};
    DrawRectangleRounded(leftRect, getRoundnessForRoundedRectangle(leftRect, 2*space), 8, col);
    DrawRectangleRounded(rightRect, getRoundnessForRoundedRectangle(leftRect, 2*space), 8, col);

    Color col1={51,59,62,255}, col2={122,176,190,255}, col3={180,182,184,255};  //col2={122,147,154,255}

    if (panningSlider) {
        Rectangle trect = sliderGetRectangle(panningSlider);
        Vector2 center = getRectangleCenter(trect);
        float tsize = trect.width, effect=sliderGetEffectValue(panningSlider);
        float panVal = sliderGetSlideValue(panningSlider);
        float r1=tsize*0.5-lerp(0.5, 1, effect)*interfaceSpace1, r2=tsize*0.5;
        Color col4=blendColors((Color){120,121,122,255}, col3, effect);
        DrawCircleV(center, r1-interfaceSpace1, col1);
        DrawRing(center, r1, r2, 150, 390, 20, col1);
        DrawRing(center, r1, r2, 150, 150+240*panVal, 20, col2);
        float angle=PI*(-7.0/6.0+4.0/3.0*panVal);
        DrawLineEx(center, (Vector2){center.x+(r1-2*interfaceSpace1)*cosf(angle), center.y+(r1-2*interfaceSpace1)*sinf(angle)}, floatMax(2, tsize*0.03), col3);

        float ty=center.y-(r2+2*interfaceSpace1)*sinf(PI*(-2.1/3.0)), tx=(r2+2*interfaceSpace1)*cosf(PI*(-2.1/3.0));
        
        renderFontStringAlign(GlobalFonts[0].font, "L", (Vector2){center.x+tx, ty}, (Vector2){0.5,0.5}, 0.7*r2, 0, col4);
        renderFontStringAlign(GlobalFonts[0].font, "R", (Vector2){center.x-tx, ty}, (Vector2){0.5,0.5}, 0.7*r2, 0, col4);
        renderFontStringAlign(GlobalFonts[0].font, "Panning", (Vector2){center.x, 0.5*(trect.y+leftRect.y)}, (Vector2){0.5,0.5}, 0.7*r2, 0, col3);
    }

    if (volumeSlider) {
        Rectangle trect = sliderGetRectangle(volumeSlider);
        Vector2 center = getRectangleCenter(trect);
        float tsize = trect.width, effect=sliderGetEffectValue(volumeSlider);
        float volVal = sliderGetSlideValue(volumeSlider);
        float r1=tsize*0.5-lerp(0.5, 1, effect)*interfaceSpace1, r2=tsize*0.5;
        Color col4=blendColors((Color){120,121,122,255}, col3, effect);
        DrawCircleV(center, r1-interfaceSpace1, col1);
        DrawRing(center, r1, r2, 150, 390, 20, col1);
        DrawRing(center, r1, r2, 150, 150+240*volVal, 20, col2);
        float angle=PI*(-7.0/6.0+4.0/3.0*volVal);
        DrawLineEx(center, (Vector2){center.x+(r1-2*interfaceSpace1)*cosf(angle), center.y+(r1-2*interfaceSpace1)*sinf(angle)}, floatMax(2, tsize*0.03), col3);

        renderFontStringAlign(GlobalFonts[0].font, TextFormat("%.2f", volVal), (Vector2){center.x, center.y-(r2+2*interfaceSpace1)*sinf(PI*(-2.1/3.0))}, (Vector2){0.5,0.5}, 0.6*r2, 0, col4);
        renderFontStringAlign(GlobalFonts[0].font, "Volume", (Vector2){center.x, 0.5*(trect.y+leftRect.y)}, (Vector2){0.5,0.5}, 0.7*r2, 0, col3);
    }
}


void renderHorizontalKeyboard() {
    renderJustHorizontalKeyboard();
    renderChangeInstrumentButton();
    renderSoundPanelEffects();
}