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



float verticalKeyboardHeight=0;
int rollKeyHovering=-1, pianoKeyHovering=-1;
Color backgroundCol1bvl={20, 20, 24, 255}, backgroundCol2bvl={22,21,25,255};



static float wkeyHeight=0, wkeyWidth=0, topPadding=15, space=0, keyScrollTarget=71.5, keyScroll=71.5, wkeyRangef=0, keyRange=0, keyRangeShown=0, blackKeyRelativeWidth=0.55, blackKeyRelativeHeight=0.5, octaveHeight=0, rollKeyHeight=0, rollStartX=0, verticalSliderX=0;
static float scrollYposMin=0, scrollYposMax=0;
static int wkeyRange=0, topKey=0, mouseInRollRect=0, mouseInKeyRect=0;
static Rectangle vkeysRect={0,0,0,0}, clipRect={0,0,0,0}, rollRect={0,0,0,0};

static float twidths[3] = {0};
static float rollKeyRelHeights[12] = {16,10,18,10,16,18,10,17,10,17,10,18}, rollKeyHeights[12]={0}, consecutiveRelOffsets[12] = {1.6, 0.3, 1.5, 0.6, 1, 1.8, 0.2, 1.5, 0.4, 1.3, 0.7, 1.1}, consecutiveOffsets[12]={0};


static Slider verticalSlider=NULL;



struct piano_roll_key {
    uint8_t key;
    uint8_t type;
    uint8_t shown;

    uint8_t rollHover;

    uint8_t pianoHover;
    uint8_t previouslyPressed;


    float pianoEffect;

    float pianoY;
    float pianoHeight;
    float rollY;
    float rollHeight;
};

static struct piano_roll_key pianoRollKeys[128]={{0,0,0,0,0,0,0,0,0,0,0}};


struct mouse_in_piano_roll {
    uint8_t inRoll;
    
    uint8_t key;
    uint8_t beat;

    uint32_t measure;
    double timestamp;

};



void verticalKeyboardInit() {
    float trelw = 0.5;
    twidths[0]=1-0.5*trelw;
    twidths[1]=1-trelw;
    twidths[2]=trelw;

    //int idxs[12]={0,2,1,2,0,0,2,1,2,1,2,0};
    //for (int i=0; i<12; i++) rollKeyRelHeights[i]=twidths[idxs[i]];

    uint8_t types[12]={0,1,0,1,0,0,1,0,1,0,1,0};

    for (int i=0; i<128; i++) {
        pianoRollKeys[i] = (struct piano_roll_key){.key=i, .type=types[i%12], .shown=0, .rollHover=0, .pianoHover=0, .previouslyPressed=0, .pianoEffect=0, .pianoY=0, .pianoHeight=0, .rollY=0, .rollHeight=0};
    }

    Rectangle rect = (Rectangle){20,20,20,20};
    verticalSlider = sliderCreate(rect, 1);
    sliderUpdateCursorOnHover(verticalSlider, MOUSE_CURSOR_RESIZE_NS);
}


void verticalKeyboardClose() {
    if (verticalSlider) sliderFree(verticalSlider);
    verticalSlider=NULL;
}


static Rectangle getClippedRect(Rectangle rect) {
    return rectangleClip(rect, clipRect);
}



void clipKeyScrollTarget() {
    scrollYposMin=keyRangeShown-3.5;
    scrollYposMax=126;
    if (keyScrollTarget>126) keyScrollTarget=126;
    if (keyScrollTarget-keyRangeShown<-3.5) keyScrollTarget=keyRangeShown-3.5;
}

void preCalculateNecessaryVerticalKeyboard() {
    space=0.5*interfaceSpace1;
    verticalKeyboardHeight = 0.7*screenSize.y;


    topPadding = floatMax(0.02*screenSize.y, 15);

    wkeyHeight = floatMax(0.03*screenSize.y, 20);
    wkeyRangef = verticalKeyboardHeight/wkeyHeight;
    wkeyRange = (int)ceil(wkeyRangef);
    wkeyHeight = (verticalKeyboardHeight-(wkeyRange-1)*space)/wkeyRange;
    wkeyWidth = floatMin(6*wkeyHeight, 0.5*trackLeftWidth);

    octaveHeight = wkeyHeight*(7+5*blackKeyRelativeHeight);
    rollKeyHeight = octaveHeight/12;
    keyRange = verticalKeyboardHeight/rollKeyHeight;

    keyRangeShown = floatMin(verticalKeyboardHeight, bottomHalfUsefulHeight-topPadding)/rollKeyHeight;
    rollStartX=trackLeftWidth+interfaceSpace1;
    verticalSliderX = screenSize.x-2*interfaceSpace1;

    for (int i=0; i<12; i++) rollKeyHeights[i]=0.1*rollKeyHeight*rollKeyRelHeights[i];
    for (int i=0; i<12; i++) consecutiveOffsets[i]=consecutiveRelOffsets[i]*rollKeyHeight;

    vkeysRect = (Rectangle){trackLeftWidth-wkeyWidth, screenSize.y-bottomHalfUsefulHeight+topPadding, wkeyWidth, floatMax(0, bottomHalfUsefulHeight-topPadding)}; //height=verticalKeyboardHeight
    mouseInKeyRect = !UIisHoveringOverLayout() && !UIisInTextInput() && !UIexistsFrontLayoutOverlay() && CheckCollisionPointRec(globalMouseHandler.pos, vkeysRect);

    clipRect = (Rectangle){0, vkeysRect.y-1, screenSize.x, screenSize.y-vkeysRect.y};

    rollRect = (Rectangle){trackLeftWidth+interfaceSpace1, screenSize.y-bottomHalfUsefulHeight+topPadding, screenSize.x-trackLeftWidth+interfaceSpace1, bottomHalfUsefulHeight-topPadding};
    mouseInRollRect = !UIisHoveringOverLayout() && !UIisInTextInput() && !UIexistsFrontLayoutOverlay() && CheckCollisionPointRec(globalMouseHandler.pos, rollRect);

}

void precomputeRollVerticalSlider();








void oldDecentKeysAndRollsUpdate() {
    
}

static int mouseHoversRoll(float y1, float y2) {
    if (!mouseInRollRect) return 0;
    return (globalMouseHandler.pos.x>=rollStartX && globalMouseHandler.pos.x<verticalSliderX && globalMouseHandler.pos.y<y2 && globalMouseHandler.pos.y>y1);
}

static int mouseHoversKey(float y1, float y2, float width) {
    if (!mouseInKeyRect) return 0;
    return (globalMouseHandler.pos.x>=vkeysRect.x && globalMouseHandler.pos.x<vkeysRect.x+width && globalMouseHandler.pos.y<y2 && globalMouseHandler.pos.y>y1);
}


static void updateRects() {
    if ((mouseInRollRect || mouseInKeyRect) && globalMouseHandler.scroll!=0 && !IsKeyDown(KEY_LEFT_CONTROL)) {
        keyScrollTarget += 2*globalMouseHandler.scroll;
    }
    clipKeyScrollTarget();

    keyScroll += 0.2*(keyScrollTarget-keyScroll);
    topKey = (int)ceil(keyScroll);

    rollKeyHovering=-1;
    pianoKeyHovering=-1;
}


void preCalculateVerticalKeyboard() {
    // Must have already ran `preCalculateNecessaryVerticalKeyboard`

    updateRects();

    float y1=screenSize.y-bottomHalfUsefulHeight+topPadding;
    float y2=y1, aligny=y1+rollKeyHeight*fmodf(keyScroll, 1.0);
    int aligni=topKey, mod=0;

    float maxY=screenSize.y, minY=screenSize.y-bottomHalfUsefulHeight+topPadding;


    for (aligni=topKey; ; aligni--) {
        mod=(aligni % 12 + 12) % 12;
        if (mod==11 || mod==4) {
            while (aligni<24) {
                aligni += 12;
                aligny-=octaveHeight;
            }
            break;
        }
        aligny+=rollKeyHeight;
    }

    if (aligni>=11) {
        y1=aligny+consecutiveOffsets[mod];
        y2=aligny+rollKeyHeight;

        for (int j=aligni-1; j>=aligni-12; j--) {
            mod=j%12;
            
            for (int i=(mod-j)/12; j+12*i<128; i++) {
                int idx=j+12*i;
                if (idx<0 || idx>127) continue;

                float ny1=y1-i*octaveHeight;
                float ny2=y2-i*octaveHeight;
                
                int shown = ((ny1<=maxY || ny2<=maxY) && (ny1+rollKeyHeights[mod]>=minY || ny2+rollKeyHeight>=minY));

                struct piano_roll_key* ck = pianoRollKeys+idx;

                ck->pianoY=ny1;
                ck->rollY=ny2;
                ck->pianoHeight=rollKeyHeights[mod];
                ck->rollHeight=rollKeyHeight;
                ck->shown=shown;

                ck->rollHover=mouseHoversRoll(ny2, ny2+rollKeyHeight);
                if (ck->rollHover) rollKeyHovering=idx;

                ck->pianoHover = mouseHoversKey(ny1, ny1+rollKeyHeights[mod], ((ck->type)?blackKeyRelativeWidth:1.0)*wkeyWidth);

                if (ck->type && ck->pianoHover) pianoKeyHovering=idx;

                if (globalMouseHandler.released && ck->previouslyPressed) globalHandlerUpdateKeyAndPlaySynth(idx, 0);

            }

            y1 += consecutiveOffsets[mod];
            y2 += rollKeyHeight;
        }
    }


    for (int i=0; i<128; i++) {
        struct piano_roll_key* ck = pianoRollKeys+i;

        if (pianoKeyHovering>=0 && !(ck->type)) ck->pianoHover=0;
        else if (pianoKeyHovering<0 && ck->pianoHover) pianoKeyHovering=i;

        ck->pianoEffect += 0.18*(ck->pianoHover-ck->pianoEffect);

        if (ck->pianoHover) setNextMouseCursor(MOUSE_CURSOR_POINTING_HAND);

        if (globalMouseHandler.pressed) {
            ck->previouslyPressed = ck->pianoHover;
            if (ck->pianoHover) globalHandlerUpdateKeyAndPlaySynth(i, (uint8_t)(127*trackGetVelocity(trackGetAtIdx(globalHandlerGetSelectedTrack()))));
        }
    }


    precomputeRollVerticalSlider();
}



void precomputeRollVerticalSlider() {
    if (!verticalSlider) return;

    Rectangle rect = {screenSize.x-2*interfaceSpace1, screenSize.y-bottomHalfUsefulHeight+topPadding, interfaceSpace1, bottomHalfUsefulHeight-topPadding-interfaceSpace1};
    sliderUpdateRectangle(verticalSlider, rect);
    if (rect.height<10) sliderDisable(verticalSlider);
    else {
        sliderEnable(verticalSlider);
        float val = (keyScroll-scrollYposMin)/(scrollYposMax-scrollYposMin);
        sliderUpdateSlideValue(verticalSlider, val);
    }
    sliderUpdate(verticalSlider, -1);

    if (isSliderDragged(verticalSlider)) {
        float th = rect.height*keyRangeShown/(scrollYposMax-scrollYposMin);
        float mposClip = floatClip(globalMouseHandler.pos.y, rect.y+0.5*th, rect.y+rect.height-0.5*th);
        float nval = 1-(mposClip-rect.y-0.5*th)/(rect.height-th);
        keyScrollTarget = nval*(scrollYposMax-scrollYposMin)+scrollYposMin;
    }

}


void renderRollVerticalSlider() {
    if (!verticalSlider || isSliderDisabled(verticalSlider)) return;

    Rectangle rect = sliderGetRectangle(verticalSlider);
    float roundness = sliderGetRoundness(verticalSlider);
    float effect = sliderGetEffectValue(verticalSlider);
    float val = sliderGetSlideValue(verticalSlider);
    Color col = blendColors((Color){100,100,100,120}, (Color){180,180,180,160}, effect);

    float th = rect.height*keyRangeShown/(scrollYposMax-scrollYposMin);
    Rectangle newRect = {rect.x, rect.y+(rect.height-th)*(1-val), rect.width, th};
    DrawRectangleRounded(newRect, roundness, 4, col);

}



static int isHorizontalLineWithinTheRect(float y) {
    return (y>clipRect.y && y<clipRect.height+clipRect.y);
}

static int isHorizontalThickLineWithinTheRect(float y, float h) {
    return isHorizontalLineWithinTheRect(y) || isHorizontalLineWithinTheRect(y+h);
}

void renderVerticalKeyboard() {
    //DrawRectangleRounded(vkeysRect, 0.1, 4, backgroundCol2bvl);
    float x=vkeysRect.x-2*interfaceSpace1, w=wkeyWidth+2*interfaceSpace1;
    float lineThickness = 0.075*rollKeyHeight, bkWmult=(wkeyWidth*blackKeyRelativeWidth+2*interfaceSpace1)/w;
    float halfLineThickness = 0.5*lineThickness;
    //float radius = getRadiusForRoundedRectangle((Rectangle){0, 0, w, pianoRollKeys[5].pianoHeight}, 0.25)-4;


    for (int j=0; j<2; j++) {
        for (int i=0; i<128; i++) {
            if (pianoRollKeys[i].type!=j) continue;

            int mod = i%12;
            if (j) {
                float ty = pianoRollKeys[i+1].pianoY-halfLineThickness;
                if (i<126 && isHorizontalLineWithinTheRect(ty)) DrawLineEx((Vector2){x, ty}, (Vector2){x+w, ty}, lineThickness, backgroundCol2bvl);
                if (mod==1 || mod==6) {
                    ty = pianoRollKeys[i-1].pianoY-halfLineThickness;
                    if (isHorizontalLineWithinTheRect(ty)) DrawLineEx((Vector2){x, ty}, (Vector2){x+w, ty}, lineThickness, backgroundCol2bvl);
                }
            }

            if (!pianoRollKeys[i].shown) continue;

            
            if (j) {
                Rectangle nrect = {x, pianoRollKeys[i].pianoY, w*bkWmult, pianoRollKeys[i].pianoHeight};
                nrect=getClippedRect(nrect);

                Color col=blendColors(COLOR_KEYBOARD_H_KEY_SELECTED_BLACK, COLOR_KEYBOARD_H_KEY_SELECTED_BLACK_HOVERED, pianoRollKeys[i].pianoEffect);
                DrawRectangleRounded(nrect, 0.25, 4, col);                

                
            } else {
                Rectangle nrect = {x, pianoRollKeys[i].pianoY, w, pianoRollKeys[i].pianoHeight};
                nrect=getClippedRect(nrect);

                Color col=blendColors(COLOR_KEYBOARD_H_KEY_SELECTED_WHITE, COLOR_KEYBOARD_H_KEY_SELECTED_WHITE_HOVERED, pianoRollKeys[i].pianoEffect);
                DrawRectangleRounded(nrect, 0.25, 4, col);

                if (!mod && isHorizontalThickLineWithinTheRect(pianoRollKeys[i].pianoY+0.15*pianoRollKeys[i].pianoHeight, 0.7*pianoRollKeys[i].pianoHeight)) {
                    renderFontStringAlign(GlobalFonts[0].font, TextFormat("C%d", i/12), (Vector2){vkeysRect.x+vkeysRect.width-interfaceSpace1*2, pianoRollKeys[i].pianoY+0.5*pianoRollKeys[i].pianoHeight}, (Vector2){1, 0.5}, 0.65*pianoRollKeys[i].pianoHeight, 0, COLOR_KEYBOARD_H_KEY_SELECTED_BLACK);
                }
            }
            //renderFontStringAlign(GlobalFonts[0].font, TextFormat("%d", pianoRollKeys[i].key), (Vector2){x+5, pianoRollKeys[i].pianoY+0.5*pianoRollKeys[i].pianoHeight}, (Vector2){0,0.5}, 20, 0, (Color){125,125,125,255});
        }
    }
}



void renderOldRollBackground() {
    int times = (int)ceil(wkeyRangef*wkeyHeight/octaveHeight*12);
    float x=trackLeftWidth+interfaceSpace1; float w=screenSize.x+2-x, y=vkeysRect.y, h=octaveHeight/12;
    char ktypes[12] = {0,1,0,1,0,0,1,0,1,0,1,0};

    Color cols[] = {(Color){25,26,30,255}, (Color){12,13,16,255}};

    for (int i=0; i<times; i++) {
        int key=topKey-i;
        int mod=key%12;
        int type=ktypes[mod];
        DrawRectangle(x, y, w, h, cols[type]);
        y+=h;
    }
}


void renderRollBackground() {
    //float x=trackLeftWidth+interfaceSpace1;
    float w=screenSize.x+2-rollStartX;
    //Color cols[] = {(Color){25,26,30,255}, (Color){12,13,16,255}};

    Color cols[] = {(Color){40,44,50,255}, (Color){24,29,32,255}, (Color){63,69,77,255}};
    Color toInterpolate = getSelectedTrackThemeColor();

    for (int i=0; i<3; i++) cols[i] = blendColors(cols[i], (Color){0,0,0,255}, 0.3);

    for (int i=0; i<128; i++) {
        if (!pianoRollKeys[i].shown) continue;

        Rectangle nrect = {rollStartX, pianoRollKeys[i].rollY, w, pianoRollKeys[i].rollHeight};
        nrect=getClippedRect(nrect);
        Color col = cols[pianoRollKeys[i].type+2*(i%12 ==0)];
        if (pianoRollKeys[i].rollHover) col = blendColors(col, toInterpolate, pianoRollKeys[i].type?0.4:0.25);
        DrawRectangleRec(nrect, col);
        if (i%12==4 && isHorizontalLineWithinTheRect(pianoRollKeys[i].rollY)) DrawLineV((Vector2){rollStartX,pianoRollKeys[i].rollY}, (Vector2){screenSize.x,pianoRollKeys[i].rollY}, cols[2]);
    }
}

void setupBackground() {
    DrawRectangle(0, screenSize.y-bottomHalfUsefulHeight, screenSize.x, bottomHalfUsefulHeight, backgroundCol1bvl);
}


static void renderOverlayToHideImperfections() {
    float x=vkeysRect.x-2*interfaceSpace1;
    DrawRectangleRec((Rectangle){x, vkeysRect.y-topPadding, interfaceSpace1*2, vkeysRect.height+topPadding}, backgroundCol1bvl);
    DrawRectangleRec((Rectangle){x, screenSize.y-bottomHalfUsefulHeight, screenSize.x-x, topPadding}, backgroundCol1bvl);

}



void renderMeasureLinesBackground() {
    double visDur = globalHandlerGetVisibleDuration();
    double ctime = globalHandlerGetTime();
    double beatDur = globalHandlerGetBeatDuration();
    double measureDur = globalHandlerGetMeasureDuration();
    int beatsInMeasure = globalHandlerGetBeatsInMeasure();

    double targetMeasures = globalHandlerDurationToMeasures(visDur);

    float startX=trackLeftWidth+interfaceSpace1, totalWidth=screenSize.x-trackLeftWidth-interfaceSpace1; float measureWidth=totalWidth/targetMeasures;
    float beatWidth = measureWidth/beatsInMeasure, clipErrorRange=1;
    int measureSkips=1, beatSkips=1, subBeats=0;

    float minMeasuresNSkipped=50, minBeatNSkipped=25;

    
    if (measureWidth<minMeasuresNSkipped) measureSkips = ceil(minMeasuresNSkipped/measureWidth);
    if (measureSkips>timelineMeasureSkipsBottom) measureSkips = timelineMeasureSkipsBottom;
    else {
        while (timelineMeasureSkipsBottom%measureSkips) measureSkips++;
    }

    if (measureSkips>1) beatSkips=beatsInMeasure*measureSkips;
    else if (beatWidth<minBeatNSkipped) {
        beatSkips = ceil(minBeatNSkipped/beatWidth);
        if (beatSkips>=beatsInMeasure) beatSkips=beatsInMeasure;
        else {
            int least=0, most=0;
            for (int i=beatSkips; i>=1; i--) {
                if (beatsInMeasure%i==0) {least=i; break;}
            }
            for (int i=beatSkips; i<=beatsInMeasure; i++) {
                if (beatsInMeasure%i==0) {most=i; break;}
            }

            if (most-beatSkips<beatSkips-least) beatSkips=most;
            else beatSkips=least;
            beatSkips=least;
        }
    }

    if (beatSkips==1) {
        subBeats=(int)round(log2(beatWidth/minBeatNSkipped));
        if (subBeats<0) subBeats=0;
        else subBeats=(1<<subBeats);
    }
    

    

    Color col1=COLOR_TEXT_1, col2=COLOR_TEXT_3, col3=COLOR_TEXT_4;
    col1.a=80, col2.a=80, col3.a=80;

    
    float y1 = rollRect.y;
    float y2 = rollRect.y+rollRect.height;


    int startMeasure = floor(globalHandlerDurationToMeasures(ctime));
    startMeasure = startMeasure/measureSkips; startMeasure=measureSkips*startMeasure;
    double start = globalHandlerMeasuresToDuration(startMeasure)-ctime;



    for (int i=0; i<targetMeasures+measureSkips+1; i+=measureSkips) {
        float posX = startX+totalWidth*(start+measureDur*i)/visDur;
        
        if (startX+totalWidth*(start+measureDur*(i+measureSkips))/visDur<startX-clipErrorRange || posX>=screenSize.x+clipErrorRange) continue;

        if (posX>=startX-clipErrorRange && posX<=screenSize.x+clipErrorRange) DrawLineEx((Vector2){posX-1, y1}, (Vector2){posX-1, y2}, 2.0, col1);


        if (subBeats) {
            int totalSubBeats = measureSkips*beatsInMeasure*subBeats;
            for (int j=1; j<totalSubBeats; j++) {
                posX = startX+totalWidth*(start+beatDur*(beatsInMeasure*i+j/(float)subBeats))/visDur;
                if (posX<startX-clipErrorRange || posX>=screenSize.x+clipErrorRange) continue;
                if (j%(beatsInMeasure*subBeats)==0) {
                    // Measure
                    DrawLineEx((Vector2){posX-1, y1}, (Vector2){posX-1, y2}, 2.0, col2);

                } else {
                    // Beat or SubBeat
                    DrawLineEx((Vector2){posX, y1}, (Vector2){posX, y2}, 1.0+0.5*(j%subBeats==0), col3);
                }
            }
        } else {
            for (int j=beatSkips; j<measureSkips*beatsInMeasure; j+=beatSkips) {
                posX = startX+totalWidth*(start+beatDur*(beatsInMeasure*i+j))/visDur;
                if (posX<startX-clipErrorRange || posX>=screenSize.x+clipErrorRange) continue;
                if (j%beatsInMeasure) {
                    // Beat
                    DrawLineEx((Vector2){posX, y1}, (Vector2){posX, y2}, 1.0, col3);
                } else if ((j/beatsInMeasure)%measureSkips==0) {
                    // Measure
                    DrawLineEx((Vector2){posX-1, y1}, (Vector2){posX-1, y2}, 2.0, col2);
                }
            }
        }
    }


}


void renderBottomKeyboardTimeLine() {
    if (!globalHandlerIsTimeLineShown()) return;

    double visDur = globalHandlerGetVisibleDuration();
    double ctime = globalHandlerGetTime();
    double ltime = globalHandlerGetLineTime();

    float startX = trackLeftWidth+interfaceSpace1, totalWidth=screenSize.x-trackLeftWidth-interfaceSpace1, y1=rollRect.y, y2=rollRect.y+rollRect.height, radius=0.08*trackCLineHeight;
    float x = startX+totalWidth/visDur*(ltime-ctime);
    y1-=radius;

    if (x<screenSize.x+radius+5 && x>startX-radius-5) {
        Color tc = COLOR_TEXT_1;
        tc.a = 120;
        float sz = 1+0.001*screenSize.x;

        float effect = buttonGetEffectValue(timeLineDragButton);

        DrawLineEx((Vector2){x, y1}, (Vector2){x, y2}, 1+sz*effect, tc);

        Color col = blendColors(COLOR_TEXT_1, COLOR_KEYBOARD_H_KEY_SELECTED_WHITE_HOVERED, effect);

        DrawLineEx((Vector2){x, y1}, (Vector2){x, y2}, 2, col);
    }
}

void renderWholeBottomLayoutTypeVertical() {
    setupBackground();
    renderRollBackground();
    renderVerticalKeyboard();

    renderMeasureLinesBackground();
    renderBottomKeyboardTimeLine();
    
    renderOverlayToHideImperfections();
    renderRollVerticalSlider();
}