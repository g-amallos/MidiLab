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
#include <export.h>



float verticalKeyboardHeight=0;
int rollKeyHovering=-1, pianoKeyHovering=-1, rollKeyBottom=-1, rollKeyTop=-1;
uint32_t mouseTimestamp=0, mouseTimestampJumps=0, mouseExactTimestamp=0, mouseExactTimestampForGroupSelection=0;
float mouseFkey=0;
Color backgroundCol1bvl={20, 20, 24, 255}, backgroundCol2bvl={22,21,25,255};

static Note rollHoveringOverNote=NULL;
static int rollHoveringOverNoteIdx=-1, rollHoverNoteType=0;

static struct {
    uint8_t key;
    uint8_t wasPressed;
    uint8_t program;
    uint8_t channel;
} releaseEvent;

// rollHoverNoteType: 0 -> not hovering | 1 -> hovering to select | 2 -> hovering to resize | 3 -> hovering to move

uint32_t controlNoteSize=(1<<13);
uint8_t controlNoteVelocity=127;


static float wkeyHeight=0, wkeyWidth=0, topPadding=15, space=0, keyScrollTarget=71.5, keyScroll=71.5, wkeyRangef=0, keyRange=0, keyRangeShown=0, blackKeyRelativeWidth=0.55, blackKeyRelativeHeight=0.5, octaveHeight=0, rollKeyHeight=0, rollStartX=0, verticalSliderX=0;
static float scrollYposMin=0, scrollYposMax=0, scrollInfoTarget=0, scrollInfoY=0;
static int wkeyRange=0, topKey=0, mouseInRollRect=0, mouseInKeyRect=0, mouseInInfoRect=0, allowClickInRollRect=0, allowClickInRect=0, mouseInBHLrect=0;
static Rectangle vkeysRect={0,0,0,0}, clipRect={0,0,0,0}, rollRect={0,0,0,0}, infoRect={0,0,0,0};

static double visibleDuration=0, divCurrentTime=0, targetMeasures=0;
static int beatsInMeasure=0; 
static uint32_t piecesInBeat=0, divStartTimestamp=0, divEndTimestamp=0;
static float divStartX=0, divTotalWidth=0, measureWidth=0, beatWidth=0;

static float twidths[3] = {0};
static float rollKeyRelHeights[12] = {16,10,18,10,16,18,10,17,10,17,10,18}, rollKeyHeights[12]={0}, consecutiveRelOffsets[12] = {1.6, 0.3, 1.5, 0.6, 1, 1.8, 0.2, 1.5, 0.4, 1.3, 0.7, 1.1}, consecutiveOffsets[12]={0};


static Slider verticalSlider=NULL, noteVelocitySlider=NULL, trackVelocitySlider=NULL, trackPanningSlider=NULL;



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

    noteVelocitySlider = sliderCreate(rect, 1);
    sliderUpdateCursorOnHover(noteVelocitySlider, MOUSE_CURSOR_RESIZE_EW);

    trackVelocitySlider = sliderCreate(rect, 1);
    sliderUpdateCursorOnHover(trackVelocitySlider, MOUSE_CURSOR_RESIZE_EW);

    trackPanningSlider = sliderCreate(rect, 1);
    sliderUpdateCursorOnHover(trackPanningSlider, MOUSE_CURSOR_RESIZE_EW);
}


void verticalKeyboardClose() {
    if (verticalSlider) sliderFree(verticalSlider);
    verticalSlider=NULL;

    if (noteVelocitySlider) sliderFree(noteVelocitySlider);
    noteVelocitySlider=NULL;

    if (trackVelocitySlider) sliderFree(trackVelocitySlider);
    trackVelocitySlider=NULL;

    if (trackPanningSlider) sliderFree(trackPanningSlider);
    trackPanningSlider=NULL;
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

static void clipInfoScrollTarget() {
    float nsize = 0.11*floatMin(vkeysRect.x, 0.4*screenSize.y);
    float fy=rollRect.y, my=textFontGetSize(GlobalFonts[0].font, "C", nsize, 0).y;
    int pairs = 10;
    float height = (3+pairs)*my+(12+pairs)*interfaceSpace1;
    if (fy+height<screenSize.y) scrollInfoTarget=0;
    else {
        if (scrollInfoTarget<0) scrollInfoTarget=0;
        else if (scrollInfoTarget>0 && fy+height-my*scrollInfoTarget<screenSize.y) scrollInfoTarget=(fy+height-screenSize.y)/my;
    }
    
}

static void updateVelocitySliders() {
    int isPlaying = globalHandlerIsPlaying();
    int isSelectedActive = globalHandlerIsSelectGroupActive();
    int notesSelected = globalHandlerGetNumberOfSelectedNotes();
    int noteVelSliderAllow = (!isSelectedActive && (!notesSelected || (notesSelected==1 && !isPlaying)));

    float nsize = 0.11*floatMin(vkeysRect.x, 0.4*screenSize.y);
    float my=textFontGetSize(GlobalFonts[0].font, "C", nsize, 0).y;
    float fy=rollRect.y-my*scrollInfoY;
    Vector2 mts;
    mts = textFontGetSize(GlobalFonts[0].font, "C", nsize, 0);
    fy += 4*mts.y+5*interfaceSpace1;

    if (noteVelocitySlider) {
        Rectangle rect = {2*interfaceSpace1, fy-1, vkeysRect.x-4*interfaceSpace1, mts.y+2};
        sliderUpdateRectangle(noteVelocitySlider, rect);

        if (noteVelSliderAllow) sliderEnable(noteVelocitySlider);
        else sliderDisable(noteVelocitySlider);

        if (mouseInInfoRect) sliderEnableHover(noteVelocitySlider);
        else sliderDisableHover(noteVelocitySlider);
        
        sliderUpdate(noteVelocitySlider, -1);

        if (isSliderDragged(noteVelocitySlider)) {
            controlNoteVelocity = (uint8_t)(127*sliderUpdateValueCommonHorizontal(noteVelocitySlider));
            if (notesSelected==1) globalHandlerChangeVelocityOfSelectedNotes(controlNoteVelocity);
        } else {
            sliderUpdateSlideValue(noteVelocitySlider, controlNoteVelocity/127.0);
        }
    }

    fy += 3*mts.y+6*interfaceSpace1;

    if (trackVelocitySlider) {
        Rectangle rect = {2*interfaceSpace1, fy-1, vkeysRect.x-4*interfaceSpace1, mts.y+2};
        sliderUpdateRectangle(trackVelocitySlider, rect);

        if (1) sliderEnable(trackVelocitySlider);
        else sliderDisable(trackVelocitySlider);
        if (mouseInInfoRect) sliderEnableHover(trackVelocitySlider);
        else sliderDisableHover(trackVelocitySlider);
        sliderUpdate(trackVelocitySlider, -1);

        if (isSliderDragged(trackVelocitySlider)) {
            trackSetVelocity(trackGetSelectedTrack(), sliderUpdateValueCommonHorizontal(trackVelocitySlider));
        } else {
            sliderUpdateSlideValue(trackVelocitySlider, trackGetVelocity(trackGetSelectedTrack()));
        }
    }
    fy += mts.y+interfaceSpace1;
    if (trackPanningSlider) {
        Rectangle rect = {2*interfaceSpace1, fy-1, vkeysRect.x-4*interfaceSpace1, mts.y+2};
        sliderUpdateRectangle(trackPanningSlider, rect);

        if (1) sliderEnable(trackPanningSlider);
        else sliderDisable(trackPanningSlider);
        if (mouseInInfoRect) sliderEnableHover(trackPanningSlider);
        else sliderDisableHover(trackPanningSlider);
        sliderUpdate(trackPanningSlider, -1);

        if (isSliderDragged(trackPanningSlider)) {
            trackSetPanning(trackGetSelectedTrack(), sliderUpdateValueCommonHorizontal(trackPanningSlider));
        } else {
            sliderUpdateSlideValue(trackPanningSlider, trackGetPanning(trackGetSelectedTrack()));
        }
    }
}

static void updateNecessaryValues();

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
    if (keyScroll-keyRangeShown<-2) rollRect.height += rollKeyHeight*(keyScroll-keyRangeShown+2);

    mouseInRollRect = !UIisHoveringOverLayout() && !UIisInTextInput() && !UIexistsFrontLayoutOverlay() && CheckCollisionPointRec(globalMouseHandler.pos, rollRect);
    enum keyboard_render_types kbt = globalStateHandlerGetKeyboardType();
    allowClickInRect = (kbt==T_KEYBOARD_VERTICAL && !(globalHandlerIsPlaying()));
    allowClickInRollRect = (allowClickInRect && mouseInRollRect);

    Rectangle bhlrect = {0, screenSize.y-bottomHalfUsefulHeight, screenSize.x, bottomHalfUsefulHeight};
    mouseInBHLrect = !UIisHoveringOverLayout() && !UIisInTextInput() && !UIexistsFrontLayoutOverlay() && CheckCollisionPointRec(globalMouseHandler.pos, bhlrect);

    if (mouseInRollRect && globalStateHandlerGetKeyboardType()==T_KEYBOARD_VERTICAL) {
        if (allowClickInRollRect) setNextMouseCursor(MOUSE_CURSOR_CROSSHAIR);
        else setNextMouseCursor(MOUSE_CURSOR_NOT_ALLOWED);
    }

    infoRect = (Rectangle){0, screenSize.y-bottomHalfUsefulHeight+topPadding, trackLeftWidth-wkeyWidth-1, bottomHalfUsefulHeight-topPadding};
    mouseInInfoRect = !UIisHoveringOverLayout() && !UIisInTextInput() && !UIexistsFrontLayoutOverlay() && CheckCollisionPointRec(globalMouseHandler.pos, infoRect);

    updateNecessaryValues();
}

static void updateNecessaryValues() {
    visibleDuration = globalHandlerGetVisibleDuration();
    divCurrentTime = globalHandlerGetTime();
    beatsInMeasure = globalHandlerGetBeatsInMeasure();
    targetMeasures = globalHandlerDurationToMeasures(visibleDuration);
    piecesInBeat = trackPiecesInBeat();
    
    divStartTimestamp = (uint32_t)(globalHandlerDurationToMeasures(divCurrentTime)*beatsInMeasure*piecesInBeat);
    divEndTimestamp = divStartTimestamp+(uint32_t)(targetMeasures*beatsInMeasure*piecesInBeat);

    divStartX=trackLeftWidth+interfaceSpace1, divTotalWidth=screenSize.x-trackLeftWidth-interfaceSpace1;
    measureWidth = divTotalWidth/targetMeasures;
    beatWidth = measureWidth/beatsInMeasure;

    if (!mouseInRollRect || !allowClickInRollRect || globalMouseHandler.pos.x<divStartX) mouseExactTimestamp=(1<<31);
    else mouseExactTimestamp=divStartTimestamp+(uint32_t)((globalMouseHandler.pos.x-divStartX)/measureWidth*beatsInMeasure*piecesInBeat);
    if (allowClickInRect) {
        if (globalMouseHandler.pos.x>=divStartX) mouseExactTimestampForGroupSelection = divStartTimestamp+(uint32_t)((globalMouseHandler.pos.x-divStartX)/measureWidth*beatsInMeasure*piecesInBeat);
        else mouseExactTimestampForGroupSelection = divStartTimestamp;
    } else mouseExactTimestampForGroupSelection=(1<<31);
    mouseFkey = -1;
}

void precomputeRollVerticalSlider();








void oldDecentKeysAndRollsUpdate() {
    
}

static int mouseHoversRoll(float y1, float y2) {
    if (!mouseInRollRect || !allowClickInRollRect) return 0;
    return (globalMouseHandler.pos.x>=rollStartX && globalMouseHandler.pos.x<verticalSliderX && globalMouseHandler.pos.y<y2 && globalMouseHandler.pos.y>y1);
}

static int mouseHoversKey(float y1, float y2, float width) {
    if (!mouseInKeyRect) return 0;
    return (globalMouseHandler.pos.x>=vkeysRect.x && globalMouseHandler.pos.x<vkeysRect.x+width && globalMouseHandler.pos.y<y2 && globalMouseHandler.pos.y>y1);
}


static void updateRects() {
    if (globalMouseHandler.scroll!=0 && !(globalMouseHandler.down) && !IsKeyDown(KEY_LEFT_CONTROL)) {
        if (mouseInRollRect || mouseInKeyRect) keyScrollTarget += 2*globalMouseHandler.scroll;
        else if (mouseInInfoRect) scrollInfoTarget -= globalMouseHandler.scroll;
    }

    clipKeyScrollTarget();

    keyScroll += 0.2*(keyScrollTarget-keyScroll);
    topKey = (int)ceil(keyScroll);

    clipInfoScrollTarget();
    scrollInfoY += 0.2*(scrollInfoTarget-scrollInfoY);

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
    float tmy=floatClip(globalMouseHandler.pos.y, y1, floatMin(screenSize.y, rollRect.y+rollRect.height));


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
                if (tmy>=ny2 && tmy<ny2+rollKeyHeight) mouseFkey=idx+(ny2+rollKeyHeight-tmy)/rollKeyHeight;

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
    updateVelocitySliders();
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

    DrawRectangleRec((Rectangle){0, vkeysRect.y, rollStartX-2, vkeysRect.height}, backgroundCol1bvl);

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

    rollKeyBottom=-1, rollKeyTop=-1;

    for (int i=0; i<128; i++) {
        if (!pianoRollKeys[i].shown) continue;
        if (rollKeyBottom!=-1) rollKeyBottom=i;
        rollKeyTop = i;

        Rectangle nrect = {rollStartX, pianoRollKeys[i].rollY, w, pianoRollKeys[i].rollHeight};
        nrect=getClippedRect(nrect);
        Color col = cols[pianoRollKeys[i].type+2*(i%12 ==0)];
        if (pianoRollKeys[i].rollHover) col = blendColors(col, toInterpolate, pianoRollKeys[i].type?0.18:0.13);
        DrawRectangleRec(nrect, col);
        if (i%12==4 && isHorizontalLineWithinTheRect(pianoRollKeys[i].rollY)) DrawLineV((Vector2){rollStartX,pianoRollKeys[i].rollY}, (Vector2){screenSize.x,pianoRollKeys[i].rollY}, cols[2]);
    }
}

void setupBackground() {
    float h=bottomHalfUsefulHeight;
    //if (keyScroll-keyRangeShown<0) {
    //    h+=rollKeyHeight*(keyScroll-keyRangeShown);
    //    //printf("keyScroll=%.3f | offset=%.3f\n", keyScroll, rollKeyHeight*(keyScroll-keyRangeShown));
    //}
    DrawRectangleRec((Rectangle){0, screenSize.y-bottomHalfUsefulHeight, screenSize.x, h}, backgroundCol1bvl);
}

static void renderOverlayToHideImperfections1() {
    float x=vkeysRect.x-2*interfaceSpace1;
    DrawRectangleRec((Rectangle){x, vkeysRect.y-topPadding, interfaceSpace1*2, vkeysRect.height+topPadding}, backgroundCol1bvl);
}

static void renderOverlayToHideImperfections2() {
    float x=vkeysRect.x-0.5*interfaceSpace1;
    DrawRectangleRec((Rectangle){x, vkeysRect.y-topPadding, interfaceSpace1*0.5, vkeysRect.height+topPadding}, backgroundCol1bvl);
    DrawRectangleRec((Rectangle){0, screenSize.y-bottomHalfUsefulHeight, screenSize.x, topPadding}, backgroundCol1bvl);

    if (keyScroll-keyRangeShown<-2) {
        float h = (keyScroll-keyRangeShown+2)*rollKeyHeight;
        DrawRectangleRec((Rectangle){0, screenSize.y+h, screenSize.x, -h}, backgroundCol1bvl);
    }
}



void renderMeasureLinesBackground() {
    double visDur = visibleDuration;
    double ctime = divCurrentTime;
    double beatDur = globalHandlerGetBeatDuration();
    double measureDur = globalHandlerGetMeasureDuration();

    
    float startX=divStartX, totalWidth=divTotalWidth;
    float clipErrorRange=1;
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

        if (subBeats>(int)piecesInBeat) subBeats=(int)piecesInBeat;
    }
    
    if (subBeats) mouseTimestampJumps = piecesInBeat/subBeats;
    else if (measureSkips==1) mouseTimestampJumps = beatSkips*piecesInBeat;
    else mouseTimestampJumps = beatSkips*piecesInBeat;

    globalHandlerSetVisibleMouseJumps(mouseTimestampJumps);


    Color col1=COLOR_TEXT_1, col2=COLOR_TEXT_3, col3=COLOR_TEXT_4;
    col1.a=80, col2.a=80, col3.a=80;

    
    float y1 = rollRect.y;
    float y2 = rollRect.y+rollRect.height;


    int startMeasure = floor(globalHandlerDurationToMeasures(ctime));
    startMeasure = startMeasure/measureSkips; startMeasure=measureSkips*startMeasure;
    double start = globalHandlerMeasuresToDuration(startMeasure)-ctime;

    //renderFontStringAlign(GlobalFonts[0].font, TextFormat("MS:%d | BS:%d | SB:%d", measureSkips, beatSkips, subBeats), (Vector2){interfaceSpace1, rollRect.y+4*interfaceSpace1}, (Vector2){0, 1}, 0.02*screenSize.x, 0, (Color){150,150,150,255});

    mouseTimestamp=(1<<30);

    for (int i=0; i<targetMeasures+measureSkips+1; i+=measureSkips) {
        float posX = startX+totalWidth*(start+measureDur*i)/visDur;
        
        if (startX+totalWidth*(start+measureDur*(i+measureSkips))/visDur<startX-clipErrorRange || posX>=screenSize.x+clipErrorRange) continue;

        if (posX>=startX-clipErrorRange && posX<=screenSize.x+clipErrorRange) DrawLineEx((Vector2){posX-1, y1}, (Vector2){posX-1, y2}, 2.0, col1);


        if (subBeats) {
            int totalSubBeats = beatsInMeasure*subBeats; //measureSkips*beatsInMeasure*subBeats
            //if (measureSkips>1) printf("MeasureSkips: %d\n", measureSkips);
            float prevPosX;
            for (int j=1; j<=totalSubBeats; j++) {
                prevPosX = posX;
                posX = startX+totalWidth*(start+beatDur*(beatsInMeasure*i+j/(float)subBeats))/visDur;
                if (posX<startX-clipErrorRange || posX>=screenSize.x+clipErrorRange) continue;
                if (j%(beatsInMeasure*subBeats)==0) {
                    // Measure
                    DrawLineEx((Vector2){posX-1, y1}, (Vector2){posX-1, y2}, 2.0, col2);

                } else {
                    // Beat or SubBeat
                    DrawLineEx((Vector2){posX, y1}, (Vector2){posX, y2}, 1.0+0.5*(j%subBeats==0), col3);
                }

                if (mouseInRollRect && allowClickInRollRect && globalMouseHandler.pos.x>=prevPosX && globalMouseHandler.pos.x<posX) {
                    mouseTimestamp = (startMeasure+i*measureSkips)*beatsInMeasure*piecesInBeat+(j-1)*mouseTimestampJumps;
                }
            }
        } else {
            float prevPosX;
            for (int j=beatSkips; j<=measureSkips*beatsInMeasure; j+=beatSkips) {
                prevPosX=posX;
                posX = startX+totalWidth*(start+beatDur*(beatsInMeasure*i+j))/visDur;
                if (posX<startX-clipErrorRange || posX>=screenSize.x+clipErrorRange) continue;
                if (j%beatsInMeasure) {
                    // Beat
                    DrawLineEx((Vector2){posX, y1}, (Vector2){posX, y2}, 1.0, col3);
                } else if ((j/beatsInMeasure)%measureSkips==0) {
                    // Measure
                    DrawLineEx((Vector2){posX-1, y1}, (Vector2){posX-1, y2}, 2.0, col2);
                }
                if (globalMouseHandler.pos.x>=prevPosX && globalMouseHandler.pos.x<posX) {
                    mouseTimestamp = piecesInBeat*(beatsInMeasure*(startMeasure+i)+(j-beatSkips));
                }
            }
        }
    }

    //renderFontStringAlign(GlobalFonts[0].font, TextFormat("MT:%d | MET:%u", mouseTimestamp/piecesInBeat, mouseExactTimestamp), (Vector2){interfaceSpace1, rollRect.y+8*interfaceSpace1}, (Vector2){0, 1}, 0.02*screenSize.x, 0, (Color){150,150,150,255});

}


static void testRenderMouseTimestamp() {
    if (!mouseInRollRect || rollKeyHovering==-1 || !allowClickInRollRect) return;

    double mtx = divStartX+(((double)mouseTimestamp)/(piecesInBeat*beatsInMeasure)-globalHandlerDurationToMeasures(divCurrentTime))*measureWidth;
    double mtw = ((double)mouseTimestampJumps)/(piecesInBeat*beatsInMeasure)*measureWidth;

    Color col = (Color){150,162,178,20};
    Rectangle rect = {mtx, rollRect.y, mtw, rollRect.height};
    DrawRectangleRec(rectangleAnd(rect, rollRect), col);

    
    rect = (Rectangle){mtx, pianoRollKeys[rollKeyHovering].rollY, mtw, pianoRollKeys[rollKeyHovering].rollHeight};
    col = blendColors(getSelectedTrackThemeColor(), (Color){200,200,200,255}, 0.25);
    col.a = 100;
    DrawRectangleRec(rectangleAnd(rect, rollRect), col);

}

static void renderWhatMightBeAdded() {
    double divOffset = globalHandlerDurationToMeasures(divCurrentTime);
    double mtx, mtw;
    Rectangle rect;
    Color interior;

    int controlDown = IsKeyDown(KEY_LEFT_CONTROL);
    int isGroupSelectionActive = globalHandlerIsSelectGroupActive();
    uint32_t selNotes = globalHandlerGetNumberOfSelectedNotes();

    if (mouseInRollRect && rollKeyHovering!=-1 && allowClickInRollRect && !controlDown && !isGroupSelectionActive && !(globalMouseHandler.down) && !(globalMouseHandler.rightClickPressed) && !rollHoveringOverNote) {
        mtx = divStartX+(((double)mouseTimestamp)/(piecesInBeat*beatsInMeasure)-divOffset)*measureWidth;
        mtw = ((double)controlNoteSize)/(piecesInBeat*beatsInMeasure)*measureWidth;

        rect = (Rectangle){mtx, pianoRollKeys[rollKeyHovering].rollY, mtw, pianoRollKeys[rollKeyHovering].rollHeight};
        interior = blendColors(getSelectedTrackThemeColor(), (Color){200,200,200,255}, 0.25);
        interior.a = 100;

        DrawRectangleRounded(rectangleAnd(rect, rollRect), 1.0, 5, interior);
    }

    if (allowClickInRect) {
        if (isGroupSelectionActive) {
            //printf("Ran\n");

            interior = blendColors(getSelectedTrackThemeColor(), (Color){180,180,180,255}, 0.25);
            struct roll_rect trect = globalHandlerGetSelectGroupRect();
            interior.a = 100;
            mtx = divStartX+(((double)(trect.topLeft.timestamp))/(piecesInBeat*beatsInMeasure)-divOffset)*measureWidth;
            mtw = ((double)(trect.bottomRight.timestamp-trect.topLeft.timestamp))/(piecesInBeat*beatsInMeasure)*measureWidth;
            int ft=(int)floor(trect.topLeft.fkey), fb=(int)floor(trect.bottomRight.fkey);
            double mty = pianoRollKeys[ft].rollY+rollKeyHeight*(1-trect.topLeft.fkey+ft);
            double mth = pianoRollKeys[fb].rollY+rollKeyHeight*(1-trect.bottomRight.fkey+fb);
            mth = mth-mty;
            rect = (Rectangle){mtx, mty, mtw, mth};
            DrawRectangleRec(rectangleAnd(rect, rollRect), interior);
        }

        if (selNotes>1) {
            interior = blendColors(getSelectedTrackThemeColor(), (Color){180,180,180,255}, 0.25);
            struct roll_rect trect = globalHandlerGetCroppedRectangleForSelectedNotes();
            interior.a = 30;
            mtx = divStartX+(((double)(trect.topLeft.timestamp))/(piecesInBeat*beatsInMeasure)-divOffset)*measureWidth;
            mtw = ((double)(trect.bottomRight.timestamp-trect.topLeft.timestamp))/(piecesInBeat*beatsInMeasure)*measureWidth;
            int ft=(int)floor(trect.topLeft.fkey), fb=(int)floor(trect.bottomRight.fkey);
            double mty = pianoRollKeys[ft].rollY+rollKeyHeight*(1-trect.topLeft.fkey+ft);
            double mth = pianoRollKeys[fb].rollY+rollKeyHeight*(1-trect.bottomRight.fkey+fb);
            mth = mth-mty;
            rect = (Rectangle){mtx, mty, mtw, mth};
            rect = rectangleAnd(rect, rectangleIncrease(rollRect, (Vector2){4,2}, (Vector2){4,2}));
            DrawRectangleRec(rect, interior);
            DrawRectangleLinesEx(rect, 2, getSelectedTrackThemeColor());
        }
    }
}

inline static int isNoteWithinBounds(uint32_t boundStart, uint32_t boundEnd, uint32_t noteStart, uint32_t noteDuration) {
    if (noteStart+noteDuration<boundStart) return 0;    // Note before window
    if (noteStart>boundEnd) return 2;                   // Note after window
    return 1;                                           // Note within window
}


inline static void renderNote(Note note, Color color, int isHovering) {
    float y=pianoRollKeys[note->key].rollY, h=pianoRollKeys[note->key].rollHeight;
    if (y+h<rollRect.y || y>rollRect.y+rollRect.height) return;

    double mtx = divStartX+(((double)(note->timestamp))/(piecesInBeat*beatsInMeasure)-globalHandlerDurationToMeasures(divCurrentTime))*measureWidth;
    double mtw = ((double)(note->duration))/(piecesInBeat*beatsInMeasure)*measureWidth;

    Rectangle rect = {mtx, pianoRollKeys[note->key].rollY, mtw, pianoRollKeys[note->key].rollHeight};
    if (isHovering) DrawRectangleRounded(rect, 0.5, 5, color);//DrawRectangleRec(rectangleAnd(rect, rollRect), color);
    else DrawRectangleRounded(rect, 0.75, 5, color);

    int space=7;

    if (isHovering && mtw>2*space) {
        rect = (Rectangle){mtx+mtw-space, pianoRollKeys[note->key].rollY+0.1*pianoRollKeys[note->key].rollHeight, 0.6*space, 0.8*pianoRollKeys[note->key].rollHeight};
        DrawRectangleRounded(rectangleAnd(rect, rollRect), 1.0, 4, blendColors(color, (Color){15,15,15,255}, 0.4));
    }
}

inline static int isHoveringOverNote(Note note) {
    return (note->key==rollKeyHovering && note->timestamp<=mouseExactTimestamp && note->timestamp+note->duration>=mouseExactTimestamp);
}

inline static int isTimestampWithinNote(Note note, uint32_t timestamp) {
    return (note->timestamp<=timestamp && timestamp-note->timestamp<note->duration);
}

inline static void _fixMouseCursorIfNeeded(int selGroupActive, int selMoveActive, int selResizeActive) {
    if (selGroupActive) setNextMouseCursor(MOUSE_CURSOR_CROSSHAIR);
    else if (selMoveActive) setNextMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
    else if (selResizeActive) setNextMouseCursor(MOUSE_CURSOR_RESIZE_EW);
}

void renderNotes() {
    Track track = trackGetSelectedTrack();
    uint32_t notesNum = trackGetNumOfNotes(track);
    Note* notes = trackGetNotes(track);

    Color colors[2] = {getTrackThemeColorForWhiteKeys(), getTrackThemeColorForBlackKeys()};
    Color tcol={0,0,0,255};

    int selGroupActive = globalHandlerIsSelectGroupActive();
    int selMoveActive = globalHandlerMoveSelectedIsActive();
    int selResizeActive = globalHandlerResizeSelectedIsActive();
    int somethingActive = (selGroupActive || selMoveActive || selResizeActive);

    rollHoveringOverNote = NULL;
    rollHoveringOverNoteIdx = -1;
    rollHoverNoteType = 0;

    int isPlaying = globalHandlerIsPlaying();
    uint32_t timelineTimestamp = globalHandlerGetLineTimestamp();

    for (uint32_t i=0; i<notesNum; i++) {
        Note note = notes[i];
        if (!note) continue;

        int within = isNoteWithinBounds(divStartTimestamp, divEndTimestamp, note->timestamp, note->duration);
        if (within==0 || note->key<rollKeyBottom || note->key>rollKeyTop) continue;
        if (within==2) break;


        tcol = colors[pianoRollKeys[note->key].type];
        int isVisuallySelected = (!isPlaying && globalHandlerIsNoteSelected(note));
        int isVisuallyHovered = (!isPlaying && !somethingActive && (!rollHoveringOverNote || isVisuallySelected) && isHoveringOverNote(note));

        if (isVisuallySelected) tcol = blendColors(tcol, (Color){230,230,230,255}, 0.5);
        else if (isVisuallyHovered) tcol = blendColors(tcol, (Color){230,230,230,255}, 0.4);
        else if (isPlaying && isTimestampWithinNote(note, timelineTimestamp)) tcol = blendColors(tcol, (Color){200,200,200,255}, 0.45);
        

        if (isVisuallyHovered) {
            rollHoveringOverNote = note;
            rollHoveringOverNoteIdx = i;
            uint32_t mn = uint32Min((mouseTimestampJumps<<1), (note->duration>>1));
            if (note->timestamp+note->duration-mouseExactTimestamp<=mn) {
                rollHoverNoteType = 2;
                setNextMouseCursor(MOUSE_CURSOR_RESIZE_EW);
            } else {
                rollHoverNoteType = 3;
                setNextMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
            }
            /*
            } else if (isVisuallySelected) {
                rollHoverNoteType = 3;
                setNextMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
            } else {
                rollHoverNoteType = 1;
                setNextMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
            }*/
        }
        renderNote(note, tcol, (rollHoveringOverNote==note)||isVisuallySelected);
    }
    _fixMouseCursorIfNeeded(selGroupActive, selMoveActive, selResizeActive);
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

static void _groupSelectionScrollIfNeeded() {
    float mx=globalMouseHandler.pos.x, my=globalMouseHandler.pos.y;
    float vx=floatClip((mx-rollRect.x)/rollRect.width, 0, 1), vy=floatClip((my-rollRect.y)/rollRect.height, 0, 1);
    double edgeX=0.15, edgeY=0.15;

    double ctime = globalHandlerGetTime();
    double cdur = globalHandlerGetVisibleDuration();

    if (vx<edgeX) {
        if (ctime>0) {
            double speed = (vx/edgeX)-1.0;
            double offset = 0.02*cdur*speed;
            globalHandlerSetTime(ctime+offset);
        } else globalHandlerSetTime(0);

    } else if (vx>1-edgeX) {
        double speed = (vx-1.0+edgeX)/edgeX;
        double offset = 0.02*cdur*speed;
        globalHandlerSetTime(ctime+offset);
    }

    if (vy<edgeY) {
        double speed = 1.0-(vy/edgeY);
        keyScrollTarget += 0.05*keyRangeShown*speed;
    } else if (vy>1-edgeY) {
        double speed = (1.0-edgeY-vy)/edgeY;
        keyScrollTarget += 0.05*keyRangeShown*speed;
    }
    clipKeyScrollTarget();
}

static void _groupSelectionScrollHorizontallyIfNeeded() {
    Note note = globalHandlerResizeSelectedGetReferenceNote();
    float mx=globalMouseHandler.pos.x;
    
    if (note) {
        double mtx = divStartX+(((double)(note->timestamp))/(piecesInBeat*beatsInMeasure)-globalHandlerDurationToMeasures(divCurrentTime))*measureWidth;
        double mtw = ((double)(note->duration))/(piecesInBeat*beatsInMeasure)*measureWidth;
        mx = mtx+mtw;
    }
    
    float vx=floatClip((mx-rollRect.x)/rollRect.width, 0, 1);
    double edgeX=0.15;

    double ctime = globalHandlerGetTime();
    double cdur = globalHandlerGetVisibleDuration();

    if (vx<edgeX) {
        if (ctime>0) {
            double speed = (vx/edgeX)-1.0;
            double offset = 0.015*cdur*speed;
            globalHandlerSetTime(ctime+offset);
        } else globalHandlerSetTime(0);

    } else if (vx>1-edgeX) {
        double speed = (vx-1.0+edgeX)/edgeX;
        double offset = 0.015*cdur*speed;
        globalHandlerSetTime(ctime+offset);
    }
}

void order2PrecomputeRoll() {
    int isPlaying = globalHandlerIsPlaying();
    int mouseDown = globalMouseHandler.down;
    int groupSelActive = globalHandlerIsSelectGroupActive();
    int groupMoveActive = globalHandlerMoveSelectedIsActive();
    int groupResizeActive = globalHandlerResizeSelectedIsActive();
    if (!isPlaying && mouseDown && (groupSelActive || groupMoveActive)) {
        _groupSelectionScrollIfNeeded();
    } else if (!isPlaying && mouseDown && groupResizeActive) {
        _groupSelectionScrollHorizontallyIfNeeded();
    }

}

static void changeControlDuration(int zx) {
    if (zx>0 && UINT32_MAX-mouseTimestampJumps>controlNoteSize) {
        controlNoteSize = mouseTimestampJumps*((controlNoteSize/mouseTimestampJumps)+1);
    } else if (zx<0 && controlNoteSize>mouseTimestampJumps) {
        uint32_t tmp = mouseTimestampJumps*((controlNoteSize/mouseTimestampJumps));
        if (tmp==controlNoteSize) tmp-=mouseTimestampJumps;
        controlNoteSize = tmp;
    } else if (zx<0 && controlNoteSize==mouseTimestampJumps && !(mouseTimestampJumps&1)) controlNoteSize = (mouseTimestampJumps>>1);
}

static Vector2 _renderSideInfoTextSeperator(const char* text, Color col, float fy, float nsize) {
    Vector2 mts = textFontGetSize(GlobalFonts[0].font, text, nsize, 0);
    if (fy>screenSize.y+1 || fy+mts.y<rollRect.y-1) return mts;

    float s=0;
    renderFontStringAlign(GlobalFonts[0].font, text, (Vector2){(vkeysRect.x-s)*0.5, fy}, (Vector2){0.5,0}, nsize, 0, blendColors(col, (Color){255,255,255,255}, 0.25));
    Vector2 p1={2*interfaceSpace1, fy+mts.y*0.5}, p2={(vkeysRect.x-s)*0.5-mts.x*0.5-interfaceSpace1, fy+mts.y*0.5}, p3={(vkeysRect.x-s)*0.5+mts.x*0.5+interfaceSpace1, fy+mts.y*0.5}, p4={vkeysRect.x-s-2*interfaceSpace1, fy+mts.y*0.5};
    DrawLineEx(p1, p2, 1.0, col);
    DrawLineEx(p3, p4, 1.0, col);

    return mts;
}

static Vector2 _renderSideInfoTextData(const char* key, const char* val, Color col, float fy, float nsize, int enabled) {
    Vector2 mts = textFontGetSize(GlobalFonts[0].font, key, nsize, 0);
    if (fy>screenSize.y+1 || fy+mts.y<rollRect.y-1) return mts;
    
    renderFontStringAlign(GlobalFonts[0].font, key, (Vector2){2*interfaceSpace1, fy}, (Vector2){0,0}, nsize, 0, enabled?COLOR_TEXT_3:COLOR_TEXT_4);
    if (enabled) renderFontStringAlign(GlobalFonts[0].font, val, (Vector2){vkeysRect.x-2*interfaceSpace1, fy}, (Vector2){1.0,0}, nsize, 0, blendColors(col, (Color){255,255,255,255}, 0.25));
    else renderFontStringAlign(GlobalFonts[0].font, "---", (Vector2){vkeysRect.x-3*interfaceSpace1, fy}, (Vector2){1,0}, nsize, 0, COLOR_TEXT_4);

    return mts;
}

static void _renderSideInfoCard(float fy, float my, int pairs, Color col) {
    Rectangle trect = {0.5*interfaceSpace1, fy, vkeysRect.x-interfaceSpace1, (1+pairs)*my+(3+pairs)*interfaceSpace1};
    if (fy>screenSize.y+1 || fy+trect.height<rollRect.y-1) return;

    trect = rectangleAnd(trect, (Rectangle){0,screenSize.y-bottomHalfHeight,screenSize.x,bottomHalfHeight});
    DrawRectangleRounded(trect, getRoundnessForRoundedRectangle(trect, 0.07*trect.width), 4, col);
}

static const char* _timestampToString(uint32_t timestamp) {
    uint32_t msr = timestamp/(beatsInMeasure*piecesInBeat);
    uint32_t bts = (timestamp-beatsInMeasure*piecesInBeat*msr)/piecesInBeat;
    float fsb = (timestamp-beatsInMeasure*piecesInBeat*msr-bts*piecesInBeat)/(double)piecesInBeat;
    return TextFormat("%u:%u:%03u", msr, bts, (uint32_t)(fsb*1000));
} 

static void renderSideInfo() {
    int isPlaying = globalHandlerIsPlaying();
    //int isSelectedActive = globalHandlerIsSelectGroupActive();
    int notesSelected = globalHandlerGetNumberOfSelectedNotes();
    uint32_t trackNotes = trackGetNumOfNotes(trackGetSelectedTrack());
    int tracksNum = projectGetTracksNum();
    uint32_t totalNotes = projectGetTotalNumberOfNotes();
    uint32_t totalProjectSize = estimateFileSizeForProject();


    int mouseInfo = (!isPlaying && mouseExactTimestamp!=(((uint32_t)1)<<31));
    int selectedInfo = (notesSelected && !isPlaying);

    float nsize = 0.11*floatMin(vkeysRect.x, 0.4*screenSize.y);
    float my=textFontGetSize(GlobalFonts[0].font, "C", nsize, 0).y, bgLerp=0.07;
    float fy=rollRect.y-my*scrollInfoY;
    Vector2 mts;
    Color backgroundCol = blendColors(COLOR_BACKGROUND_1, blendColors(COLOR_TRACK_THEME_0, COLOR_TRACK_THEME_1, 0.5), bgLerp);

    _renderSideInfoCard(fy, my, 4, backgroundCol);
    fy += interfaceSpace1;

    mts = _renderSideInfoTextSeperator("Control", COLOR_TRACK_THEME_0, fy, nsize);
    fy += mts.y+interfaceSpace1;

    const char* mnf = NULL;
    if (mouseInfo) mnf=_timestampToString(mouseExactTimestamp);
    mts = _renderSideInfoTextData("Mouse:", mouseInfo?mnf:NULL, COLOR_TRACK_THEME_1, fy, nsize, mouseInfo);
    fy += mts.y+interfaceSpace1;
    mts = _renderSideInfoTextData("Selected:", selectedInfo?(TextFormat("%u", notesSelected)):NULL, COLOR_TRACK_THEME_1, fy, nsize, selectedInfo);
    fy += mts.y+interfaceSpace1;
    mnf = NULL;
    if (mouseInfo) mnf=_timestampToString(controlNoteSize);
    mts = _renderSideInfoTextData("Length:", mouseInfo?mnf:NULL, COLOR_TRACK_THEME_2, fy, nsize, mouseInfo);
    fy += mts.y+interfaceSpace1;

    if (noteVelocitySlider && fy<screenSize.y+1 && fy+mts.y>rollRect.y-1) {
        float sldEffect = sliderGetEffectValue(noteVelocitySlider);
        int isDisabled = isSliderDisabled(noteVelocitySlider);
        Color sldbg;
        Color sldfg;
        if (isDisabled) {
            sldbg = (Color){9, 10, 13, 255};
            sldfg = blendColors(COLOR_TRACK_THEME_2, (Color){20,20,20,255}, 0.65);
        } else {
            sldbg = (Color){12, 13, 19, 255};
            sldfg = blendColors(COLOR_TRACK_THEME_2, (Color){0,0,0,255}, 0.35-0.15*sldEffect);
        }
        Rectangle trc = sliderGetRectangle(noteVelocitySlider);
        DrawRectangleRounded(trc, 0.3, 4, sldbg);
        DrawRectangleRounded(sliderGetRectangleValueCommon(noteVelocitySlider), 0.3, 4, sldfg);

        renderFontStringAlign(GlobalFonts[0].font, "Volume", getRectangleCenter(trc), (Vector2){0.5,0.5}, nsize, 0, isDisabled?COLOR_TEXT_4:blendColors(COLOR_TEXT_3, COLOR_TEXT_1, 0.7*sldEffect));
    }
    fy += mts.y+3*interfaceSpace1;

    backgroundCol = blendColors(COLOR_BACKGROUND_1, blendColors(COLOR_TRACK_THEME_3, COLOR_TRACK_THEME_4, 0.5), bgLerp);
    _renderSideInfoCard(fy, my, 3, backgroundCol);
    fy += interfaceSpace1;

    mts = _renderSideInfoTextSeperator("Track", COLOR_TRACK_THEME_3, fy, nsize);
    fy += mts.y+interfaceSpace1;

    mts = _renderSideInfoTextData("Notes:", TextFormat("%u", trackNotes), COLOR_TRACK_THEME_4, fy, nsize, 1);
    fy += mts.y+interfaceSpace1;

    if (trackVelocitySlider && fy<screenSize.y+1 && fy+mts.y>rollRect.y-1) {
        float sldEffect = sliderGetEffectValue(trackVelocitySlider);
        Color sldbg={12,13,19,255};
        Color sldfg=blendColors(COLOR_TRACK_THEME_4, (Color){0,0,0,255}, 0.35-0.15*sldEffect);
        Rectangle trc = sliderGetRectangle(trackVelocitySlider);
        DrawRectangleRounded(trc, 0.3, 4, sldbg);
        DrawRectangleRounded(sliderGetRectangleValueCommon(trackVelocitySlider), 0.3, 4, sldfg);
    
        renderFontStringAlign(GlobalFonts[0].font, "Volume", getRectangleCenter(trc), (Vector2){0.5,0.5}, nsize, 0, blendColors(COLOR_TEXT_3, COLOR_TEXT_1, 0.7*sldEffect));
    }
    fy += mts.y+interfaceSpace1;

    if (trackPanningSlider && fy<screenSize.y+1 && fy+mts.y>rollRect.y-1) {
        float sldEffect = sliderGetEffectValue(trackPanningSlider);
        Color sldbg={12,13,19,255};
        Color sldfg=blendColors(COLOR_TRACK_THEME_4, (Color){0,0,0,255}, 0.35-0.15*sldEffect);
        Color sldtc=blendColors(COLOR_TEXT_3, COLOR_TEXT_1, 0.7*sldEffect);
        Rectangle trc = sliderGetRectangle(trackPanningSlider);
        DrawRectangleRounded(trc, 0.3, 4, sldbg);
        DrawRectangleRounded(sliderGetRectangleValueCommon(trackPanningSlider), 0.3, 4, sldfg);

        renderFontStringAlign(GlobalFonts[0].font, "L", (Vector2){trc.x+0.5*interfaceSpace1, trc.y+trc.height*0.5}, (Vector2){0,0.5}, nsize, 0, sldtc);
        renderFontStringAlign(GlobalFonts[0].font, "R", (Vector2){trc.x+trc.width-0.5*interfaceSpace1, trc.y+trc.height*0.5}, (Vector2){1,0.5}, nsize, 0, sldtc);
        renderFontStringAlign(GlobalFonts[0].font, "Panning", getRectangleCenter(trc), (Vector2){0.5,0.5}, nsize, 0, sldtc);
    }

    fy += mts.y+3*interfaceSpace1;

    backgroundCol = blendColors(COLOR_BACKGROUND_1, blendColors(COLOR_TRACK_THEME_5, COLOR_TRACK_THEME_6, 0.5), bgLerp);
    _renderSideInfoCard(fy, my, 3, backgroundCol);
    fy += interfaceSpace1;

    mts = _renderSideInfoTextSeperator("Project", COLOR_TRACK_THEME_5, fy, nsize);
    fy += mts.y+interfaceSpace1;

    mts = _renderSideInfoTextData("Tracks:", TextFormat("%u", tracksNum), COLOR_TRACK_THEME_6, fy, nsize, 1);
    fy += mts.y+interfaceSpace1;

    mts = _renderSideInfoTextData("Notes:", TextFormat("%u", totalNotes), COLOR_TRACK_THEME_6, fy, nsize, 1);
    fy += mts.y+interfaceSpace1;

    mnf = NULL;
    if (totalProjectSize<(1<<10)) mnf = TextFormat("%u B", totalProjectSize);
    else if (totalProjectSize<(1<<20)) mnf = TextFormat("%.1f KB", totalProjectSize/1024.0);
    else if (totalProjectSize<(1<<30)) mnf = TextFormat("%.1f MB", totalProjectSize/1048576.0);
    else mnf = TextFormat("%.2fGB", (totalProjectSize/1048576.0)/1024.0);

    mts = _renderSideInfoTextData("Size:", mnf, COLOR_TRACK_THEME_6, fy, nsize, 1);
    fy += mts.y+interfaceSpace1;

    
}


static void handlerIntermediateEvents() {
    if (globalHandlerIsPlaying() && globalHandlerIsSelectGroupActive()) globalHandlerSelectGroupClear();

    int allowedShortcuts = (!UIisHoveringOverLayout() && !UIisInTextInput());
    int controlDown = IsKeyDown(KEY_LEFT_CONTROL);
    int delPressed = IsKeyPressed(KEY_DELETE);
    int aPressed = IsKeyPressed(KEY_A);
    int cPressed = IsKeyPressed(KEY_C);
    int xPressed = IsKeyPressed(KEY_X);
    int vPressed = IsKeyPressed(KEY_V);


    if (allowedShortcuts && delPressed) globalHandlerDeleteSelectedNotes();
    else if (allowedShortcuts && aPressed && controlDown) actionDefer(globalHandlerSelectAllNotes);
    else if (allowedShortcuts && cPressed && controlDown) actionDefer(globalHandlerCopySelected);
    else if (allowedShortcuts && xPressed && controlDown) actionDefer(globalHandlerCutSelected);
    else if (allowedShortcuts && vPressed && controlDown) actionDefer(globalHandlerPasteSelected);



    int zx = IsKeyPressed(KEY_D)-IsKeyPressed(KEY_A);
    if (zx && allowedShortcuts && !controlDown) changeControlDuration(zx);
}

static void _selectSingleNote(Note note) {
    globalHandlerSelectSingleNote(note);
    controlNoteVelocity = note->velocity;
    controlNoteSize = note->duration;
}

static void handleClick() {
    if (globalMouseHandler.released) {
        if (releaseEvent.wasPressed) {
            releaseEvent.wasPressed = 0;
            synthProgramNoteOffPanning(releaseEvent.key, releaseEvent.program, globalHandlerGetSelectedTrack());
        }
    }

    int controlDown = IsKeyDown(KEY_LEFT_CONTROL);
    //int selectedNotes = globalHandlerGetNumberOfSelectedNotes();
    int selGroupActive = globalHandlerIsSelectGroupActive();
    int selMoveActive = globalHandlerMoveSelectedIsActive();
    int selResizeActive = globalHandlerResizeSelectedIsActive();



    if (globalMouseHandler.pressed) {
        printf("mouseInRollRect: %d | allowClickInRollRect: %d | mouseInBHLrect: %d\n", mouseInRollRect, allowClickInRollRect, mouseInBHLrect);
        if (mouseInRollRect && allowClickInRollRect) {
            //printf("Clicked: mouseInRollRect=%d, mouseTimestamp=%d, rollKeyHovering=%d\n", mouseInRollRect, mouseTimestamp, rollKeyHovering);
            if (mouseInRollRect && mouseTimestamp!=(1<<30) && rollKeyHovering!=-1) {
                //printf("Hovering over note: %p | idx=%d\n", (void*)rollHoveringOverNote, rollHoveringOverNoteIdx);
                if (rollHoveringOverNote) {
                    if (controlDown) globalHandlerToggleSelectedNote(rollHoveringOverNote);
                    //else if (selectedNotes) {
                        if (rollHoverNoteType==3) { // Move
                            if (!globalHandlerIsNoteSelected(rollHoveringOverNote)) _selectSingleNote(rollHoveringOverNote);
                            globalHandlerMoveSelectedPress(rollHoveringOverNote, mouseFkey, mouseExactTimestampForGroupSelection);
                        } else if (rollHoverNoteType==2) {  // Resize
                            if (!globalHandlerIsNoteSelected(rollHoveringOverNote)) _selectSingleNote(rollHoveringOverNote);
                            globalHandlerResizeSelectedPress(rollHoveringOverNote, mouseFkey, mouseExactTimestampForGroupSelection);
                        } else if (rollHoverNoteType==1) {
                            _selectSingleNote(rollHoveringOverNote);
                        }
                    //} else {
                    //    _selectSingleNote(rollHoveringOverNote);
                    //}
                } else if (controlDown) {
                    globalHandlerSelectGroupPress(mouseFkey, mouseExactTimestampForGroupSelection);
                } else {
                    Track track = trackGetSelectedTrack();
                    uint8_t program = trackGetProgram(track);
                    Note nt = trackCreateNoteInTrack(track, rollKeyHovering, controlNoteVelocity, mouseTimestamp, controlNoteSize);
                    synthProgramNoteOnPanning(rollKeyHovering, 0.007874*controlNoteVelocity*trackGetVelocity(track), program, trackGetPanning(track), globalHandlerGetSelectedTrack());

                    releaseEvent.key = rollKeyHovering;
                    releaseEvent.wasPressed = 1;
                    releaseEvent.program = program;
                    releaseEvent.channel = 0;

                    if (controlDown) globalHandlerAddNoteToSelected(nt);
                    else globalHandlerClearNotesSelected();
                }
            }
        }
    } else if (globalMouseHandler.down) {
        if (selGroupActive) globalHandlerSelectGroupHold(mouseFkey, mouseExactTimestampForGroupSelection);
        else if (selMoveActive) globalHandlerMoveSelectedHold(mouseFkey, mouseExactTimestampForGroupSelection);
        else if (selResizeActive) globalHandlerResizeSelectedHold(mouseFkey, mouseExactTimestampForGroupSelection);

    } else if (globalMouseHandler.released) {
        if (selGroupActive) globalHandlerSelectGroupRelease(mouseFkey, mouseExactTimestampForGroupSelection);
        if (selMoveActive) globalHandlerMoveSelectedRelease(mouseFkey, mouseExactTimestampForGroupSelection);
        if (selResizeActive) controlNoteSize = globalHandlerResizeSelectedRelease(mouseFkey, mouseExactTimestampForGroupSelection);

    }
    
    if (globalMouseHandler.rightClickPressed) {
        if (mouseInRollRect && allowClickInRollRect && mouseTimestamp!=(1<<30) && rollKeyHovering!=-1) {
            if (rollHoveringOverNote) {
                globalHandlerRemoveSelectedNote(rollHoveringOverNote);
                trackDeleteNoteInTrackByIdx(trackGetSelectedTrack(), rollHoveringOverNoteIdx);
            }
        }
    }
}

void renderWholeBottomLayoutTypeVertical() {
    setupBackground();
    renderRollBackground();
    

    renderMeasureLinesBackground();
    renderBottomKeyboardTimeLine();

    testRenderMouseTimestamp();

    renderNotes();

    renderWhatMightBeAdded();

    renderVerticalKeyboard();
    renderOverlayToHideImperfections1();
    
    renderSideInfo();
    renderOverlayToHideImperfections2();
    renderRollVerticalSlider();
    

    handlerIntermediateEvents();
    handleClick();
}