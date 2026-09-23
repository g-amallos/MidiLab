#include <raylib.h>
#include <utils.h>
#include <interface.h>
#include <backend.h>
#include <ui.h>
#include <colors.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "tiles.h"


#define NUM_OF_INT_SETTINGS 5
#define NUM_OF_FLOAT_SETTINGS 5
#define NUM_OF_SETTINGS 10


static Button backButton=NULL;
static Color tilesBackgroundColor={0,0,0,0}, disabledBackgroundColor={0,0,0,0}, textOrHoverBackgroundColor={0,0,0,0}, settingsBackgroundColor={0,0,0,0};
static Rectangle settingsDiv={0,0,0,0}, controlDiv={0,0,0,0}, previewDiv={0,0,0,0}, previewRect={0,0,0,0};
static struct tiles_settings settings;
static float settingsSeperatorX=0, divsPixelRadius=0;
static Button settingsDivBtns[NUM_OF_SETTINGS]={NULL}, settingsLeftRightBtns[NUM_OF_INT_SETTINGS][2]={{NULL}};
static Slider settingsFloatSliders[NUM_OF_FLOAT_SETTINGS]={NULL}, timeSlider=NULL;
static int beatsInMeasure=4, isPlaying=0;
static double beatDuration=1.0, previewAspectRatio=9.0/16.0, curTime=0;
static struct view_data view;


static struct duration _generateDuration(enum setting_bool timeNotation, int integer, double floatingPoint) {
    struct duration ret;
    if (timeNotation) {
        ret.beats.beats = integer;
        ret.beats.remainder = (float)fmod(floatingPoint, 1.0);
        ret.seconds = (integer+ret.beats.remainder)*beatDuration;
    } else {
        ret.seconds = floatingPoint;
        double fbeats = floatingPoint/beatDuration;
        ret.beats.beats = (int)floor(fbeats);
        ret.beats.remainder = fbeats-ret.beats.beats;
    }
    return ret;
}



static void _updateViewColumns() {
    if (!view.columns) return;

    float blackKeyWidth=0.5, w=view.whiteKeyPerc;
    float w1=w-0.5*w*blackKeyWidth, w2=w*blackKeyWidth, w3=w*(1.0-blackKeyWidth);
    float offsets[] = {w1,w2,w3,w2,w1,w1,w2,w3,w2,w3,w2,w1};

    float x=view.spacePerc;
    for (int i=settings.info.minShownKey; i<=settings.info.maxShownKey; i++) {
        int mod = i%12;
        view.columns[i-settings.info.minShownKey] = (struct note_column){.key=i, .x=x, .w=offsets[mod]};
        //printf("column: #%d: x=%.3f, w=%.3f\n", i, x, offsets[mod]);
        x += offsets[mod];
    }
}

static void _updateView() {
    int needsReallocation = (view.totalKeys!=settings.info.shownKeys);
    //if (needsReallocation) printf("_updateView: needs reallocation\n");
    view.totalKeys = settings.info.shownKeys;
    view.spacePerc = settings.space;
    view.keyHeight = settings.keyHeight;
    view.whiteKeyPerc = (1.0-2*view.spacePerc)/view.whiteKeys;

    if (needsReallocation) {
        if (view.columns) free(view.columns);
        view.columns = malloc(sizeof(struct note_column)*view.totalKeys);
    }

    if (view.columns) _updateViewColumns();

}

static void _updateInfoChunk(int drumsChanged) {
    if (drumsChanged) settings.info = projectGetTilesInfo(settings.showDrums);
    settings.totalDuration = _generateDuration(0, 0, settings.startDelay.seconds+settings.endDelay.seconds+settings.info.duration);
    
    if (drumsChanged) {
        uint8_t octave[] = {0,1,0,1,0,0,1,0,1,0,1,0};
        int whiteKeys=0;
        for (uint8_t i=settings.info.minShownKey; i<=settings.info.maxShownKey; i++) {
            whiteKeys += !(octave[i%12]);
        }

        view.whiteKeys = whiteKeys;
    }

    if (drumsChanged) {
        if (settings.notes.notes) projectFreeNoteArray(settings.notes);
        settings.notes = projectGetNoteArray(settings.showDrums);
    }

    _updateView();
}

void verticalTilesInit() {
    Rectangle rect = {0,0,20,20};

    if (backButton) buttonFree(backButton);
    backButton = buttonCreate(rect, 0.25);

    beatsInMeasure = globalHandlerGetBeatsInMeasure();
    beatDuration = globalHandlerGetBeatDuration();

    settings = (struct tiles_settings) {
        .beatsInMeasure = beatsInMeasure,

        .theme = THEME_PROJECT,
        .timeNotation = SETTING_ON,     // 0 -> seconds, 1 -> beats 
        .countMeasures = SETTING_ON,
        .identifyChords = SETTING_OFF,
        .showDrums = SETTING_OFF,
        
        .delayConstraints = (struct float_constraints){.min=0, .max=beatDuration*beatsInMeasure*4},
        .startDelay = _generateDuration(1, beatsInMeasure, 0.0),
        .endDelay = _generateDuration(1, beatsInMeasure, 0.0),

        .visDurConstraints = (struct float_constraints){.min=beatDuration*beatsInMeasure, .max=beatDuration*beatsInMeasure*10},
        .visibleDuration = _generateDuration(1, beatsInMeasure*3, 0.0),

        .keyHeightConstraints = (struct float_constraints){.min=0.05, .max=0.2},
        .keyHeight = 0.1,

        .spacingConstraints = (struct float_constraints){.min=0, .max=0.1},
        .space = 0.05,

    };

    view = (struct view_data){
        .totalKeys=0,
        .whiteKeys=0,
        .spacePerc = 0.02,
        .whiteKeyPerc = 0.1,
        .keyHeight = 0.1,
        .blackKeyWidth=0.55,
        .blackKeyHeight=0.6,

        .columns=NULL,
    };

    for (int i=0; i<NUM_OF_INT_SETTINGS; i++) {
        if (settingsLeftRightBtns[i][0]) buttonFree(settingsLeftRightBtns[i][0]);
        if (settingsLeftRightBtns[i][1]) buttonFree(settingsLeftRightBtns[i][1]);
        settingsLeftRightBtns[i][0] = buttonCreate(rect, 0.25);
        settingsLeftRightBtns[i][1] = buttonCreate(rect, 0.25);
    }

    for (int i=0; i<NUM_OF_SETTINGS; i++) {
        if (settingsDivBtns[i]) buttonFree(settingsDivBtns[i]);
        settingsDivBtns[i] = buttonCreate(rect, 0.25);
    }

    for (int i=0; i<NUM_OF_FLOAT_SETTINGS; i++) {
        if (settingsFloatSliders[i]) sliderFree(settingsFloatSliders[i]);
        settingsFloatSliders[i] = sliderCreate(rect, 0.35);
        sliderUpdateCursorOnHover(settingsFloatSliders[i], MOUSE_CURSOR_RESIZE_EW);
    }

    if (timeSlider) sliderFree(timeSlider);
    timeSlider = sliderCreate(rect, 1.0);
    isPlaying=0;

    _updateInfoChunk(1);
}

void closeTileSettings() {
    if (settings.notes.notes) projectFreeNoteArray(settings.notes);
    settings.notes.size = 0;
    settings.notes.notes = NULL;

    if (view.columns) free(view.columns);
    view.columns = NULL;
}

void verticalTilesClose() {
    if (backButton) buttonFree(backButton);
    backButton = NULL;

    for (int i=0; i<NUM_OF_INT_SETTINGS; i++) {
        if (settingsLeftRightBtns[i][0]) buttonFree(settingsLeftRightBtns[i][0]);
        if (settingsLeftRightBtns[i][1]) buttonFree(settingsLeftRightBtns[i][1]);
        settingsLeftRightBtns[i][0] = NULL;
        settingsLeftRightBtns[i][1] = NULL;
    }

    for (int i=0; i<NUM_OF_SETTINGS; i++) {
        if (settingsDivBtns[i]) buttonFree(settingsDivBtns[i]);
        settingsDivBtns[i] = NULL;
    }

    for (int i=0; i<NUM_OF_FLOAT_SETTINGS; i++) {
        if (settingsFloatSliders[i]) sliderFree(settingsFloatSliders[i]);
        settingsFloatSliders[i] = NULL;
    }

    if (timeSlider) sliderFree(timeSlider);
    timeSlider = NULL;



    closeTileSettings();
}




static const char* _getSettingStringForTheme() {
    switch (settings.theme) {
        case THEME_PROJECT: return "MidiLab";
        case THEME_CLASSIC: return "Classic";
        case THEME_CLEAN: return "Clean";
        case THEME_TEST: return "Test";
        default: return "Unavailable";
    }
}

static const char* _getSettingStringForBool(enum setting_bool sbool) {
    switch (sbool) {
        case SETTING_OFF: return "Off";
        case SETTING_ON: return "On";
        default: return "Unavailable";
    }
}

static char* _getSettingsStringForDuration(struct duration duration) {   // Must be later freed
    if (settings.timeNotation) {
        int tmp = duration.beats.beats;
        int measures=tmp/settings.beatsInMeasure;
        int beats=tmp-settings.beatsInMeasure*measures;
        return strdup(TextFormat("%d:%u:%03u", measures, (uint32_t)beats, (uint32_t)(1000*duration.beats.remainder)));
    } else {
        return strdup(TextFormat("%.3lf", duration.seconds));
    }
}

static char* _getSettingStringForUint32(uint32_t n) {   // Must be later freed
    return strdup(TextFormat("%u", n));
}

static char* _getSettingStringForFloat(double val) {    // Must be later freed
    return strdup(TextFormat("%.3lf", val));
}


static void _settingArrowPressed(int idx, int leftRight) {
    if (idx<0 || leftRight<0 || leftRight>1) return;
    switch (idx) {
        case 0: {
            if (leftRight) settings.theme = (settings.theme+1)%THEME_END;
            else settings.theme = settings.theme?(settings.theme-1):THEME_END-1;
            return;
        }

        case 1: {
            settings.timeNotation = !(settings.timeNotation);
            return;
        }

        case 2: {
            settings.countMeasures = !(settings.countMeasures);
            return;
        }

        case 3: {
            settings.identifyChords = !(settings.identifyChords);
            return;
        }

        case 4: {
            settings.showDrums = !(settings.showDrums);
            _updateInfoChunk(1);
            return;
        }

        default: return;
    }
}

static void someSettingButtonPressed() {
    for (int i=0; i<NUM_OF_INT_SETTINGS; i++) {
        Button btn1=settingsLeftRightBtns[i][0], btn2=settingsLeftRightBtns[i][1];

        if (btn1 && isButtonClicked(btn1)) _settingArrowPressed(i, 0);
        if (btn2 && isButtonClicked(btn2)) _settingArrowPressed(i, 1);
    }
}

static void _settingsFloatDragged(int idx, int released) {
    if (idx<0 || idx>=NUM_OF_FLOAT_SETTINGS) return;
    double min=0, max=1;
    switch (idx) {
        case 0:
        case 1: {
            min=settings.delayConstraints.min;
            max=settings.delayConstraints.max;
            break;
        }
        case 2: {
            min=settings.visDurConstraints.min;
            max=settings.visDurConstraints.max;
            break;
        }
        case 3: {
            min=settings.keyHeightConstraints.min;
            max=settings.keyHeightConstraints.max;
            break;
        }
        case 4: {
            min=settings.spacingConstraints.min;
            max=settings.spacingConstraints.max;
            break;
        }
        default: break;
    }

    double val = lerp(min, max, sliderGetSlideValue(settingsFloatSliders[idx]));
    switch (idx) {
        case 0: {
            settings.startDelay=_generateDuration(0,0, val);
            break;
        }
        case 1: {
            settings.endDelay=_generateDuration(0,0, val);
            break;
        }
        case 2: {
            settings.visibleDuration=_generateDuration(0,0, val);
            break;
        }
        case 3: {
            settings.keyHeight = val;
            break;
        }
        case 4: {
            settings.space = val;
            break;
        }
        default: break;
    }
    if (released && idx<2) _updateInfoChunk(0);
    else if (idx<5) _updateView();
}

static void _settingsFloatReleased(int idx) {
    if (idx<0 || idx>=NUM_OF_FLOAT_SETTINGS) return;
    if (!settings.timeNotation) {
        _settingsFloatDragged(idx, 1);
        return;
    }

    double min=0, max=1, target;
    switch (idx) {
        case 0:
        case 1: {
            min=settings.delayConstraints.min;
            max=settings.delayConstraints.max;
            break;
        }
        case 2: {
            min=settings.visDurConstraints.min;
            max=settings.visDurConstraints.max;
            break;
        }
        default: break;
    }

    if (idx<3) {
        double val = lerp(min, max, sliderGetSlideValue(settingsFloatSliders[idx]));
        val = beatDuration*round(val/beatDuration);
        target = doubleClip(val, min, max);
    }


    switch (idx) {
        case 0: {
            settings.startDelay=_generateDuration(0,0, target);
            break;
        }
        case 1: {
            settings.endDelay=_generateDuration(0,0, target);
            break;
        }
        case 2: {
            settings.visibleDuration=_generateDuration(0,0, target);
            break;
        }
        default: break;
    }
    if (idx<2) _updateInfoChunk(0);
}

static float _settingsFloatGetNormalizedVal(int idx) {
    switch (idx) {
        case 0: return (settings.startDelay.seconds-settings.delayConstraints.min)/(settings.delayConstraints.max-settings.delayConstraints.min);
        case 1: return (settings.endDelay.seconds-settings.delayConstraints.min)/(settings.delayConstraints.max-settings.delayConstraints.min);
        case 2: return (settings.visibleDuration.seconds-settings.visDurConstraints.min)/(settings.visDurConstraints.max-settings.visDurConstraints.min);
        case 3: return (settings.keyHeight-settings.keyHeightConstraints.min)/(settings.keyHeightConstraints.max-settings.keyHeightConstraints.min);
        case 4: return (settings.space-settings.spacingConstraints.min)/(settings.spacingConstraints.max-settings.spacingConstraints.min);
        
        default: return 0;
    }
}

static void _backToRegularRender() {
    verticalTilesClose();
    globalHandlerSetRenderType(ART_REGULAR);
}

static void _updateTilesButtons() {
    //float tknprc = 0.75;
    float l = controlLineHeight;
    if (backButton) {
        Rectangle rect = {0.2*l, 0.2*l, 0.8*l, 0.8*l};
        //float x=renderTypeRect.x+0.5*(1.0-tknprc)*renderTypeRect.height, y=renderTypeRect.y+0.5*(1.0-tknprc)*renderTypeRect.height, w=tknprc*renderTypeRect.height;
        //Rectangle rect = {x, y, w, w};
        //printf("Rect: x=%.1lf, y=%.1lf, w=%.1lf, h=%.1lf\n", rect.x, rect.y, rect.width, rect.height);
        buttonUpdateRectangle(backButton, rect);
        buttonUpdate(backButton, -1);
        if (isButtonClicked(backButton)) actionDefer(_backToRegularRender);
    }


    float rh=0.05*screenSize.y;
    Rectangle fill = {interfaceSpace1,settingsDiv.y+interfaceSpace2+rh+interfaceSpace1,settingsDiv.width-2*interfaceSpace1,rh};
    for (int i=0; i<NUM_OF_SETTINGS; i++) {
        Button btn = settingsDivBtns[i];
        if (btn) {
            buttonUpdateRectangle(btn, fill);
            buttonUpdate(btn, -1);
        }
        fill.y += rh+interfaceSpace1;
    }

    float y=settingsDiv.y+interfaceSpace2+rh+interfaceSpace1, w=rh-2*interfaceSpace1;
    for (int i=0; i<NUM_OF_INT_SETTINGS; i++) {
        Button btn1=settingsLeftRightBtns[i][0], btn2=settingsLeftRightBtns[i][1];

        if (btn1) {
            Rectangle rect = {settingsSeperatorX+interfaceSpace1, y+interfaceSpace1, w, w};
            buttonUpdateRectangle(btn1, rect);
            buttonUpdate(btn1, -1);
            if (isButtonClicked(btn1)) actionDefer(someSettingButtonPressed);
        }

        if (btn2) {
            Rectangle rect = {settingsDiv.x+settingsDiv.width-interfaceSpace1-w, y+interfaceSpace1, w, w};
            buttonUpdateRectangle(btn2, rect);
            buttonUpdate(btn2, -1);
            if (isButtonClicked(btn2)) actionDefer(someSettingButtonPressed);
        }
        y += rh+interfaceSpace1;
    }
    for (int i=0; i<NUM_OF_FLOAT_SETTINGS; i++) {
        Slider sld = settingsFloatSliders[i];
        if (sld) {
            Rectangle rect={settingsSeperatorX,y,settingsDiv.x+settingsDiv.width-settingsSeperatorX,rh};
            sliderUpdateRectangle(sld, rect);
            sliderUpdate(sld, -1);

            if (isSliderDragged(sld)) {
                sliderUpdateValueCommonHorizontal(sld);
                _settingsFloatDragged(i, 0);
            } else if (isSliderReleased(sld)) {
                sliderUpdateValueCommonHorizontal(sld);
                _settingsFloatReleased(i);
            } else {
                sliderUpdateSlideValue(sld, _settingsFloatGetNormalizedVal(i));
            }
        }
        y += rh+interfaceSpace1;
    }


    if (timeSlider) {
        float h = floatMax(5,0.007*screenSize.y);
        Rectangle rect={controlDiv.x,controlDiv.y+controlDiv.height-0.5*h,controlDiv.width,h};
        sliderUpdateRectangle(timeSlider, rect);
        if (isPlaying || settings.totalDuration.seconds<=0) sliderDisable(timeSlider);
        else sliderEnable(timeSlider);
        sliderUpdate(timeSlider, -1);
        if (isSliderDragged(timeSlider)) curTime = settings.totalDuration.seconds*sliderUpdateValueCommonHorizontal(timeSlider);
        else sliderUpdateSlideValue(timeSlider, curTime/settings.totalDuration.seconds);
    }
    
}

static void _updateTilesValues() {

    tilesBackgroundColor = blendColors(COLOR_BACKGROUND_3, COLOR_BACKGROUND_1, 0.3);
    disabledBackgroundColor = blendColors(tilesBackgroundColor, BLACK, 0.2);
    textOrHoverBackgroundColor = blendColors(tilesBackgroundColor, WHITE, 0.03);
    settingsBackgroundColor = blendColors(COLOR_BACKGROUND_3, WHITE, 0.01);

    controlDiv = (Rectangle){0, 0, screenSize.x, controlLineHeight*1.2};
    settingsDiv = (Rectangle){0, controlDiv.y+controlDiv.height+interfaceSpace2, 0.35*screenSize.x, screenSize.y-(controlDiv.y+controlDiv.height+interfaceSpace2)};
    settingsSeperatorX = settingsDiv.x+0.5*settingsDiv.width;
    previewDiv = (Rectangle){settingsDiv.x+settingsDiv.width+interfaceSpace2, settingsDiv.y, screenSize.x-(settingsDiv.x+settingsDiv.width+interfaceSpace2), settingsDiv.height};
    previewRect = rectangleScaleToFitInCenter((Vector2){(float)previewAspectRatio,1.0}, previewDiv);

    double dt = GetFrameTime();
    if (isPlaying) curTime+=dt;
    curTime = doubleClip(curTime, 0.0, settings.totalDuration.seconds);
}

void order2precomputeVerticalTiles() {
    _updateTilesValues();
    _updateTilesButtons();
}

static float _getTextSizeToFitInRect(Rectangle rect, float defaultSize, const char* text, float spacing) {
    if (!text) return defaultSize;

    float sizeH = textFontGetSize(GlobalFonts[0].font, "0A", defaultSize, 0).y;
    float sizeX = textFontGetSize(GlobalFonts[0].font, text, defaultSize, 0).x;
    float finalSize = defaultSize;
    if (sizeX>rect.width-spacing) finalSize=floatMin(finalSize, defaultSize*(rect.width-spacing)/sizeX);
    if (sizeH>rect.width-spacing) finalSize=floatMin(finalSize, defaultSize*(rect.height-spacing)/sizeH);
    return finalSize;
}

static void _renderBackButton() {
    if (!backButton) return;

    float effect = buttonGetEffectValue(backButton);
    Color col1=textOrHoverBackgroundColor, col2={32, 34, 40, 255};
    Color blendedCol = blendColors(col1, col2, effect);

    Rectangle rect = buttonGetRectangle(backButton);
    Rectangle trect = scaleRctangleFromCenter(rect, lerp(0.3, 1, effect));
    DrawRectangleRounded(trect, buttonGetRoundness(backButton), 4, blendedCol);
    iconRerder(T_ICON_UNDO, scaleRctangleFromCenter(rect, 0.8*lerp(0.85, 1, effect)), blendColors(COLOR_TEXT_1, COLOR_PALETTE_1_P9, effect));
}

static void _renderTimeSlider() {
    if (!timeSlider) return;

    Rectangle rect = sliderGetRectangle(timeSlider);
    Rectangle trect = sliderGetRectangleValueCommon(timeSlider);

    DrawRectangleRec(rect, settingsBackgroundColor);
    DrawRectangleRec(trect, COLOR_TRACK_THEME_5);
}

static void _renderControlDiv() {
    DrawRectangleRec(controlDiv, tilesBackgroundColor);
    _renderBackButton();
    _renderTimeSlider();
}


void _renderFeaturePlaceholder() {
    Rectangle rect1 = {0, 0, screenSize.x, controlLineHeight*1.2};
    DrawRectangleRec(rect1, tilesBackgroundColor);

    float textSize = 0.05*floatMin(screenSize.x, screenSize.y);
    renderFontStringAlign(GlobalFonts[0].font, "Settings Placeholder", getRectangleCenter(rect1), (Vector2){0.5,0.5}, 0.85*textSize, 0, COLOR_TEXT_1);

    float tx = 0.05*floatMin(screenSize.x, screenSize.y);
    Rectangle rect2 = {tx, rect1.y+rect1.height+tx, screenSize.x-2*tx, screenSize.y-rect1.y-rect1.height-2*tx};
    DrawRectangleRounded(rect2, 0.08, 8, tilesBackgroundColor);
    renderFontStringAlign(GlobalFonts[0].font, "Preview Placeholder", getRectangleCenter(rect2), (Vector2){0.5,0.5}, textSize, 0, COLOR_TEXT_1);
}


static void _renderKeyValuePair(const char* key, const char* value, float textSize, Button btn, enum setting_type type, int option, int options, void* btns) {
    if (!btn) return;

    Rectangle bkgRect = buttonGetRectangle(btn);    //{fill.x+interfaceSpace1, fill.y, fill.width-2*interfaceSpace1, fill.height};
    float divEffect=buttonGetEffectValue(btn);
    float y=bkgRect.y+0.5*bkgRect.height, x1=bkgRect.x+0.5*interfaceSpace2;// x2=bkgRect.x+bkgRect.width-0.5*interfaceSpace2;
    float pixRadius = getRadiusForRoundedRectangle(bkgRect, 0.35);
    Color bkgCol = blendColors(settingsBackgroundColor, WHITE, 0.02*divEffect);
    DrawRectangleRounded(bkgRect, 0.35, 4, bkgCol);

    if (key) {
        Rectangle tmp = (Rectangle){bkgRect.x, bkgRect.y, settingsSeperatorX-bkgRect.x, bkgRect.height};
        float finalSize = _getTextSizeToFitInRect(tmp, textSize, key, interfaceSpace2);
        renderFontStringAlign(GlobalFonts[0].font, key, (Vector2){x1,y}, (Vector2){0, 0.5}, finalSize, 0, blendColors(COLOR_TEXT_3, COLOR_TEXT_1, divEffect));
    }

    if (value) {
        Rectangle tmp = (Rectangle){settingsSeperatorX, bkgRect.y, bkgRect.x+bkgRect.width-settingsSeperatorX, bkgRect.height};

        if (type==SETTING_BOOL || type==SETTING_INT) {
            float totalWidth = 0.5*tmp.width;
            float tmSpacing = floatMin(0.2*totalWidth/options, interfaceSpace1), y=tmp.y+tmp.height-0.75*interfaceSpace1, h=0.5*interfaceSpace1;
            float optionWidth = (totalWidth-tmSpacing*(options-1))/options, x=tmp.x+0.5*tmp.width-0.5*totalWidth;
            Color palette[2] = {COLOR_PALETTE_1_P6, COLOR_TEXT_2};    //blendColors(COLOR_PALETTE_1_P9, settingsBackgroundColor, 0.25)

            for (int i=0; i<options; i++) {
                int sel = (option==i);
                DrawRectangleRounded((Rectangle){x,y,optionWidth,h}, 1.0, 4, palette[sel]);
                x += optionWidth+tmSpacing;
            }

            if (btns) {
                enum icon_title icons[2] = {T_ICON_LEFT, T_ICON_RIGHT};

                for (int i=0; i<2; i++) {
                    Button btn=((Button*)btns)[i];
                    if (!btn) continue;
                    Rectangle rect = buttonGetRectangle(btn);
                    float effect = buttonGetEffectValue(btn);
                    iconRerder(icons[i], scaleRctangleFromCenter(rect, lerp(0.6, 0.8, effect)), blendColors(palette[1], COLOR_PALETTE_1_P9, effect));
                }
            }
        } else if (type==SETTING_FLOAT) {
            if (btns || *(Slider*)btns) {
                Slider sld = ((Slider*)btns)[0];
                float effect = sliderGetEffectValue(sld);
                float val = sliderGetSlideValue(sld);

                Rectangle trect = {tmp.x,tmp.y,floatMin(tmp.width*val, tmp.width-pixRadius),tmp.height};
                Color col = blendColors(bkgCol, COLOR_PALETTE_1_P3, 0.2+0.8*effect);
                DrawRectangleRec(trect, col);
                if (tmp.width*val>tmp.width-pixRadius) {
                    Rectangle tempRect = {tmp.x+tmp.width-2*pixRadius+tmp.width*(val-1.0),tmp.y,2*pixRadius,tmp.height};
                    DrawRectangleRounded(tempRect,1.0,4,col);
                }
            
                
            }

        }


        float finalSize = _getTextSizeToFitInRect(tmp, textSize, value, interfaceSpace2);
        renderFontStringAlign(GlobalFonts[0].font, value, getRectangleCenter(tmp), (Vector2){0.5,0.5}, finalSize, 0, blendColors(COLOR_TEXT_1, COLOR_TEXT_2, divEffect));  //(Vector2){x2,y}, (Vector2){1, 0.5}
    }
    DrawLineEx((Vector2){settingsSeperatorX, bkgRect.y}, (Vector2){settingsSeperatorX, bkgRect.y+bkgRect.height}, 2.0, COLOR_TEXT_4);
}


static void _renderInfoKeyValuePair(const char* key, const char* value, float textSize, Rectangle fill) {
    Rectangle bkgRect = {fill.x+interfaceSpace1, fill.y, fill.width-2*interfaceSpace1, fill.height};
    float y=bkgRect.y+0.5*bkgRect.height, x1=bkgRect.x+0.5*interfaceSpace2;// x2=bkgRect.x+bkgRect.width-0.5*interfaceSpace2;
    Color bkgCol = settingsBackgroundColor;
    DrawRectangleRounded(bkgRect, 0.35, 4, bkgCol);

    if (key) {
        Rectangle tmp = (Rectangle){bkgRect.x, bkgRect.y, settingsSeperatorX-bkgRect.x, bkgRect.height};
        float finalSize = _getTextSizeToFitInRect(tmp, textSize, key, interfaceSpace2);
        renderFontStringAlign(GlobalFonts[0].font, key, (Vector2){x1,y}, (Vector2){0, 0.5}, finalSize, 0, COLOR_TEXT_3);
    }

    if (value) {
        Rectangle tmp = (Rectangle){settingsSeperatorX, bkgRect.y, bkgRect.x+bkgRect.width-settingsSeperatorX, bkgRect.height};
        float finalSize = _getTextSizeToFitInRect(tmp, textSize, value, interfaceSpace2);
        renderFontStringAlign(GlobalFonts[0].font, value, getRectangleCenter(tmp), (Vector2){0.5,0.5}, finalSize, 0, COLOR_TEXT_3);  //(Vector2){x2,y}, (Vector2){1, 0.5}
    }
    DrawLineEx((Vector2){settingsSeperatorX, bkgRect.y}, (Vector2){settingsSeperatorX, bkgRect.y+bkgRect.height}, 2.0, COLOR_TEXT_4);
}

static void _renderSettingsTitle(const char* title, float textSize, Rectangle fill) {
    Rectangle bkgRect = {fill.x+interfaceSpace1, fill.y, fill.width-2*interfaceSpace1, fill.height};
    float y=bkgRect.y+0.5*bkgRect.height, x=bkgRect.x+0.5*bkgRect.width;
    float finalSize = _getTextSizeToFitInRect(bkgRect, textSize, title, interfaceSpace2);
    renderFontStringAlign(GlobalFonts[0].font, title, (Vector2){x,y}, (Vector2){0.5, 0.5}, finalSize, 0, COLOR_TEXT_1);
}

static void _renderSettings() {
    Rectangle tempRect = {-0.5*settingsDiv.width,settingsDiv.y,settingsDiv.width*1.5,settingsDiv.height*1.5};
    divsPixelRadius = getRadiusForRoundedRectangle(tempRect, 0.08);
    DrawRectangleRounded(tempRect, 0.08, 8, tilesBackgroundColor);

    float textSize = 0.032*floatMin(screenSize.x, screenSize.y);
    float rh=0.05*screenSize.y;
    Rectangle rect = {0,settingsDiv.y+interfaceSpace2,settingsDiv.width,rh};
    _renderSettingsTitle("Settings", textSize, rect);
    rect.y += rh+interfaceSpace1;

    const char* keys[] = {"Theme", "Time Notation", "Count Measures", "Identify Chords", "Show Drums", "Start Delay", "End Delay", "Visible Duration", "Key Height", "Spacing"};
    const char* vals[] = {_getSettingStringForTheme(), settings.timeNotation?"Beats":"Seconds", _getSettingStringForBool(settings.countMeasures), _getSettingStringForBool(settings.identifyChords), _getSettingStringForBool(settings.showDrums), _getSettingsStringForDuration(settings.startDelay), _getSettingsStringForDuration(settings.endDelay), _getSettingsStringForDuration(settings.visibleDuration), _getSettingStringForFloat(settings.keyHeight), _getSettingStringForFloat(settings.space)};
    enum setting_type types[] = {SETTING_INT, SETTING_BOOL, SETTING_BOOL, SETTING_BOOL, SETTING_BOOL, SETTING_FLOAT, SETTING_FLOAT, SETTING_FLOAT,SETTING_FLOAT,SETTING_FLOAT};
    int numOfOptions[] = {THEME_END,2,2,2,2, 0,0,0,0,0};
    int selectedOptions[] = {settings.theme, settings.timeNotation, settings.countMeasures, settings.identifyChords, settings.showDrums, 0,0,0,0,0};
    int s = intMin(sizeof(keys)/sizeof(const char*), sizeof(vals)/sizeof(const char*));
    
    for (int i=0; i<s; i++) {
        void* pointer = NULL;
        if (i<NUM_OF_INT_SETTINGS) pointer = settingsLeftRightBtns[i];
        else if (i<NUM_OF_INT_SETTINGS+NUM_OF_FLOAT_SETTINGS) pointer = settingsFloatSliders+(i-NUM_OF_INT_SETTINGS);

        _renderKeyValuePair(keys[i], vals[i], textSize, settingsDivBtns[i], types[i], selectedOptions[i], numOfOptions[i], pointer);
        rect.y += rh+interfaceSpace1;
    }
    for (int i=NUM_OF_INT_SETTINGS; i<NUM_OF_SETTINGS; i++) {
        if (vals[i]) free((char*)vals[i]);
        vals[i]=NULL;
    }


    if (0) {
        const char* nkeys[] = {"Total Duration", "Visible Keys", "Rendered Tracks", "Rendered Notes"};
        char* nvals[] = {_getSettingsStringForDuration(settings.totalDuration), _getSettingStringForUint32(settings.info.shownKeys), _getSettingStringForUint32(settings.info.tracks), _getSettingStringForUint32(settings.info.notes)};
        s = intMin(sizeof(nkeys)/sizeof(const char*), sizeof(nvals)/sizeof(char*));
        rect.y = settingsDiv.y+settingsDiv.height-interfaceSpace2-(s+1)*rh-s*interfaceSpace1;
        _renderSettingsTitle("Information", textSize, rect);
        rect.y += rh+interfaceSpace1;
        

        for (int i=0; i<s; i++) {
            _renderInfoKeyValuePair(nkeys[i], nvals[i], textSize, rect);
            if (nvals[i]) free(nvals[i]);
            nvals[i]=NULL;
            rect.y += rh+interfaceSpace1;
        }
    }
    
}

static void _renderPianoKeysClassic() {
    float heighKeyPerc=view.keyHeight, blackKeyHeight=view.blackKeyHeight, blackKeyWidth=view.blackKeyWidth;
    float y=previewRect.y+previewRect.height*(1.0-heighKeyPerc)-view.spacePerc*previewRect.width, w=view.whiteKeyPerc*previewRect.width, h=heighKeyPerc*previewRect.height;
    //DrawRectangleRec((Rectangle){x,y,width,heighKeyPerc*previewRect.height}, (Color){200,200,200,255});
    Color cols[] = {(Color){200,200,200,255}, (Color){25,25,25,255}};

    uint8_t keyType[] = {0,1,0,1,0,0,1,0,1,0,1,0};
    for (int iteration=0; iteration<2; iteration++) {
        float x=previewRect.x+view.spacePerc*previewRect.width+iteration*(w-0.5*blackKeyWidth*w);
        for (int i=settings.info.minShownKey; i<=settings.info.maxShownKey; i++) {
            int mod = i%12;
            uint8_t type = keyType[mod];
            if (type!=iteration) {
                if (mod==11 || mod==4) x+=w;
                continue;
            }

            if (iteration==0) {
                Rectangle rect = {x+1,y,w-2,h};
                DrawRectangleRec(rect, cols[type]);
            } else {
                Rectangle rect = {x,y,w*blackKeyWidth,h*blackKeyHeight};
                DrawRectangleRec(rect, cols[type]);
            }
            x += w;
        }
    }

    int gradient=0;
    if (gradient) {
        Color grcol[] = {(Color){160,63,156,60}, (Color){116,50,110,0}};
        float x=previewRect.x+view.spacePerc*previewRect.width, d=floatMax(0.5*w, 0.02*previewRect.height);
        DrawRectangleGradientEx((Rectangle){x,y-d,w*view.whiteKeys,d}, grcol[1], grcol[0], grcol[0], grcol[1]);
    }

}



static void _renderRollBackgroundStripes() {
    float heighKeyPerc=view.keyHeight, blackKeyWidth=0.5;//view.blackKeyWidth;
    float y=previewRect.y, w=view.whiteKeyPerc*previewRect.width, h=previewRect.height*(1.0-heighKeyPerc)-view.spacePerc*previewRect.width;
    float w1=w-0.5*w*blackKeyWidth, w2=w*blackKeyWidth, w3=w*(1.0-blackKeyWidth);
    float offsets[] = {w1,w2,w3,w2,w1,w1,w2,w3,w2,w3,w2,w1};
    uint8_t keyType[] = {0,1,0,1,0,0,1,0,1,0,1,0};
    Color cols[] = {(Color){40,44,50,255}, (Color){24,29,32,255}, (Color){75,80,88,255}};
    for (int i=0; i<2; i++) cols[i] = blendColors(cols[i], (Color){0,0,0,255}, 0.3);

    float x=previewRect.x+view.spacePerc*previewRect.width;
    for (int i=settings.info.minShownKey; i<=settings.info.maxShownKey; i++) {
        int mod = i%12;
        uint8_t type = keyType[mod];
        
        Rectangle rect = {x,y,offsets[mod],h};
        DrawRectangleRec(rect, cols[type]);
        if (!mod && i>settings.info.minShownKey) DrawLineV((Vector2){x,y}, (Vector2){x,y+h}, cols[2]);
        x += offsets[mod];
    }
}

static void _renderRollBackgroundSingle() {
    
}

static void _renderRollBackgroundBeatBreaks() {
    //printf("_renderRollBackgroundBeatBreaks: entered\n");
    int iterations=3+(settings.visibleDuration.seconds/beatDuration);
    int start = floor((curTime-settings.startDelay.seconds)/beatDuration);

    float topY=previewRect.y, x1=previewRect.x+view.spacePerc*previewRect.width, x2=previewRect.x+(1.0-view.spacePerc)*previewRect.width, bottomY=previewRect.y+previewRect.height*(1.0-view.keyHeight)-view.spacePerc*previewRect.width;
    float range = bottomY-topY;
    float textSize = floatMin(range*beatDuration/settings.visibleDuration.seconds, previewRect.width*0.05);

    for (int i=0; i<iterations; i++) {
        int itr = i+start;
        if (itr<0) continue;

        float y=bottomY-range*(itr*beatDuration-curTime+settings.startDelay.seconds)/settings.visibleDuration.seconds;
        
        if (y<topY-1) break;

        //printf("Iteration: #%d, i=%d, y=%lf\n", itr, i, y);
        if (y<=bottomY+1) DrawLineEx((Vector2){x1,y}, (Vector2){x2,y}, 0.5+1.0*(itr%beatsInMeasure==0), COLOR_TEXT_5);
        if (settings.countMeasures) {
            if (y-0.9*textSize>bottomY+1) continue;
            float textY = y-0.1*textSize;
            renderFontStringAlign(GlobalFonts[0].font, TextFormat("%u:%u", 1+itr/beatsInMeasure, itr%beatsInMeasure), (Vector2){x1+0.1*textSize, textY}, (Vector2){0,1.0}, 0.8*textSize, 0, COLOR_TEXT_4);
            if (textY<topY-1) break;
        } else if (y<topY-1) break;
    }
}

static void _renderPianoRoll() {
    switch (settings.theme) {
        case THEME_CLASSIC: {
            _renderPianoKeysClassic();
            return;
        }
        default: break;
    }
}

static void _renderRollBackground() {
    DrawRectangleRec(previewRect, COLOR_BACKGROUND_1);

    switch (settings.theme) {
        case THEME_PROJECT:
        case THEME_CLASSIC: {
            _renderRollBackgroundStripes();
            break;
        }
        default: {
            _renderRollBackgroundSingle();
            break;
        };
    }
    _renderRollBackgroundBeatBreaks();
}


static void _renderNotesProject() {
    float topY=previewRect.y, bottomY=previewRect.y+previewRect.height*(1.0-view.keyHeight)-view.spacePerc*previewRect.width;
    float range = bottomY-topY;

    //printf("`_renderNotesProject`: arr=%p, n=%llu, columns=%p\n", (void*)settings.notes.notes, settings.notes.size, (void*)view.columns);
    uint64_t n=settings.notes.size;
    Note* arr = settings.notes.notes;
    if (!arr || !n || !view.columns) return;

    uint8_t types[]={0,1,0,1,0,0,1,0,1,0,1,0};

    for (uint64_t i=0; i<n; i++) {
        Note nt = arr[i];

        float y1=bottomY-range*(nt->ftimestamp-curTime+settings.startDelay.seconds)/settings.visibleDuration.seconds;
        float y2=bottomY-range*(nt->ftimestamp+nt->fduration-curTime+settings.startDelay.seconds)/settings.visibleDuration.seconds;
        int key = nt->key;
        int idx = key-settings.info.minShownKey;
        if (y2>bottomY+1) continue;
        if (y1<topY-1) break;

        //DrawRectangleRec((Rectangle){previewRect.x+previewRect.width*view.columns[idx].x, y2, previewRect.width*view.columns[idx].w, y1-y2}, getTrackThemeColor(nt->track));
        Rectangle rect = {previewRect.x+previewRect.width*view.columns[idx].x+1, y2, previewRect.width*view.columns[idx].w-2, y1-y2};
        Color col = (types[key%12]?getAnyTrackThemeColorForBlackKeys(nt->track):getAnyTrackThemeColorForWhiteKeys(nt->track));
        DrawRectangleRounded(rect, 1.0, 4, col);
    }
}


static void _renderNotesInRoll() {
    switch (settings.theme) {
        case THEME_PROJECT: {
            break;
        }
        case THEME_CLASSIC: {
            break;
        }
        default: {
            break;
        };
    }
    _renderNotesProject();
}

static void _renderPreviewImperfectionOverlay() {
    float y=previewRect.y+previewRect.height*(1.0-view.keyHeight)-view.spacePerc*previewRect.width, h=view.keyHeight*previewRect.height+view.spacePerc*previewRect.width;
    DrawRectangleRec((Rectangle){previewRect.x,y,previewRect.width,h}, COLOR_BACKGROUND_1);
}

static void _renderPreviewDiv() {
    DrawRectangleRounded(previewDiv, getRoundnessForRoundedRectangle(previewDiv, divsPixelRadius), 8, tilesBackgroundColor);
    


    _renderRollBackground();
    _renderNotesInRoll();
    _renderPreviewImperfectionOverlay();
    _renderPianoRoll();
}

static void _hideImperfections() {
    DrawRectangleRec((Rectangle){0,0,screenSize.x,previewDiv.y}, COLOR_BACKGROUND_1);
}

void renderVerticalTiles() {
    //_renderFeaturePlaceholder();
    _renderSettings();
    _renderPreviewDiv();
    _hideImperfections();
    _renderControlDiv();
}