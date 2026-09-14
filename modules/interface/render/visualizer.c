#include <interface.h>
#include <backend.h>
#include <ui.h>
#include <colors.h>
#include <utils.h>
#include <visualizer.h>
#include <math.h>
#include <string.h>


#define PROJECT_TITLE_PLACEHOLDER "Project Title"


static Button backButton=NULL, playPauseButton=NULL;
static Slider progressSlider=NULL;
static Rectangle visualizerRect={0,0,0,0}, progressRect={0,0,0,0}, controlRect={0,0,0,0}, renderTypeRect={0,0,0,0};
static float visualizerRectRadius=0, visualizerRectRoundness=0.08;
static struct duration_data duration={0,0};
static Color visualizerBackgroundColor={0,0,0,0};
static int progressWasHolding=0, progressWasPlaying=0;



void visualizerInit() {
    Rectangle rect = {0,0,20,20};

    if (backButton) buttonFree(backButton);
    backButton = buttonCreate(rect, 0.25);

    if (playPauseButton) buttonFree(playPauseButton);
    playPauseButton = buttonCreate(rect, 0.25);

    if (progressSlider) sliderFree(progressSlider);
    progressSlider = sliderCreate(rect, 0.5);

    duration = projectGetDuration();
}



void visualizerClose() {
    if (backButton) buttonFree(backButton);
    backButton=NULL;
    if (playPauseButton) buttonFree(playPauseButton);
    playPauseButton=NULL;
    if (progressSlider) sliderFree(progressSlider);
    progressSlider=NULL;
}

static void _backToRegularRender() {
    visualizerClose();
    globalHandlerSetRenderType(ART_REGULAR);
}

struct duration_data visualizerGetDurationData() {
    if (backButton) return duration;
    return (struct duration_data){0,0};
}


static void _updateVisualizerButtons() {
    float l = controlLineHeight;
    if (backButton) {
        Rectangle rect = {0.2*l, 0.2*l, 0.8*l, 0.8*l};
        buttonUpdateRectangle(backButton, rect);
        buttonUpdate(backButton, -1);
        if (isButtonClicked(backButton)) actionDefer(_backToRegularRender);
    }

    if (progressSlider) {
        Rectangle rect = {progressRect.x+interfaceSpace1, progressRect.y+interfaceSpace1, progressRect.width-2*interfaceSpace1, progressRect.height-2*interfaceSpace1};
        sliderUpdateRectangle(progressSlider, rect);
        if (duration.timestamp>0) sliderEnable(progressSlider);
        else sliderDisable(progressSlider);
        
        sliderUpdate(progressSlider, -1);
        if (duration.timestamp>0 && isSliderDragged(progressSlider)) {
            int isPlaying = globalHandlerIsPlaying();
            if (progressWasHolding==0) {
                progressWasHolding=1;
                progressWasPlaying=isPlaying;
            }
            if (isPlaying) {
                globalHandlerPause();
            }
            double val = duration.time*sliderUpdateValueCommonHorizontal(progressSlider);
            globalHandlerSetLineTime(val);
        } else {
            if (duration.timestamp>0) {
                sliderUpdateSlideValue(progressSlider, floatClip(globalHandlerGetLineTime()/duration.time, 0.0, 1.0));
                if (progressWasHolding) {
                    progressWasHolding = 0;
                    if (progressWasPlaying) globalHandlerPlay();
                    progressWasPlaying=0;
                }
            }
            else sliderUpdateSlideValue(progressSlider, 0);
        }
    }
}

static void _updateVisualizerValues() {
    float tx = 0.05*floatMin(screenSize.x, screenSize.y);
    visualizerRect = (Rectangle){tx, screenSize.y*0.4, screenSize.x-2*tx, screenSize.y*0.6-tx};
    visualizerRectRadius = getRadiusForRoundedRectangle(visualizerRect, visualizerRectRoundness);

    visualizerBackgroundColor = blendColors(COLOR_BACKGROUND_3, COLOR_BACKGROUND_1, 0.3);

    progressRect = (Rectangle){visualizerRect.x, visualizerRect.y-0.85*controlLineHeight, visualizerRect.width, 0.85*controlLineHeight-interfaceSpace1};

    controlRect = (Rectangle){visualizerRect.x, progressRect.y-controlLineHeight-interfaceSpace1, visualizerRect.width, controlLineHeight};
    renderTypeRect = (Rectangle){visualizerRect.x, controlRect.y-controlLineHeight-interfaceSpace1, visualizerRect.width, controlLineHeight};
}


void order2PrecomputeVisualizer() {
    if (globalHandlerGetRenderType()!=ART_VISUALIZER) return;

    _updateVisualizerValues();
    _updateVisualizerButtons();
    
    FFTupdate();
}



static void _renderBackButton() {
    if (!backButton) return;

    float effect = buttonGetEffectValue(backButton);
    Color col1 = {20, 21, 23, 0}, col2={28, 30, 34, 255};
    Color blendedCol = blendColors(col1, col2, effect);

    Rectangle rect = buttonGetRectangle(backButton);
    DrawRectangleRounded(scaleRctangleFromCenter(rect, lerp(0.3, 1, effect)), buttonGetRoundness(backButton), 8, blendedCol);
    iconRerder(T_ICON_UNDO, scaleRctangleFromCenter(rect, 0.8*lerp(0.85, 1, effect)), blendColors(COLOR_TEXT_1, COLOR_PALETTE_1_P9, effect));
}

float _getAverage(float* buff, int num) {
    float pr = 1.0/num;
    float ret=0;
    for (int i=0; i<num; i++) ret += buff[i];
    return pr*ret;
}

static Color _getCol(float s) {
    Color cols[7] = {COLOR_TRACK_THEME_0, COLOR_TRACK_THEME_1, COLOR_TRACK_THEME_2, COLOR_TRACK_THEME_3, COLOR_TRACK_THEME_4, COLOR_TRACK_THEME_5, COLOR_TRACK_THEME_6};
    s = floatClip(s, 0, 6);
    if (s<=0) return cols[0];
    if (s>=6) return cols[6];
    int idx = (int)floor(s);
    return blendColors(cols[idx], cols[idx+1], s-idx);
}

static void _renderFFTVisualizer() {
    DrawRectangleRounded(visualizerRect, visualizerRectRoundness, 4, visualizerBackgroundColor);
    float pxRad = visualizerRectRadius;

    int num=0;
    float* buffer = FFTgetIntensityBuffer(&num);
    if (num<=0) return;

    int steps = 2;
    int iters = num/steps;
    float w=(visualizerRect.width-2*pxRad)/iters, x=visualizerRect.x+pxRad, y=visualizerRect.y+visualizerRect.height-pxRad, h=visualizerRect.height-2*pxRad;
    float w2=w*0.5;
    float lg2b10 = 1.0/log10(2.0);

    for (int i=0; i<iters; i++) {
        float v = lg2b10*log10(floatClip(sqrt(buffer[i]*51.2/num), 0.0, 1.0)+1.0);
        DrawRectangleRec((Rectangle){x+0.5*w2, y-h*v, w2, h*v}, _getCol(7.0*v));
        x += w;
    }
}

static void _renderVisualizerDecorations() {
    DrawRectangleRounded(renderTypeRect, getRoundnessForRoundedRectangle(renderTypeRect, visualizerRectRadius), 4, visualizerBackgroundColor);

    float textSize = 0.045*floatMin(screenSize.x, screenSize.y);
    renderFontStringAlign(GlobalFonts[0].font, "FFT Spectum Visualizer", getRectangleCenter(renderTypeRect), (Vector2){0.5, 0.5}, textSize, 0, COLOR_TEXT_1);
}


static void _renderVisualizerProgress() {
    if (!progressSlider) return;

    float effect = sliderGetEffectValue(progressSlider);
    DrawRectangleRounded(progressRect, getRoundnessForRoundedRectangle(progressRect, visualizerRectRadius), 4, blendColors(visualizerBackgroundColor, COLOR_BACKGROUND_3, effect*0.8));
    Rectangle rect = sliderGetRectangle(progressSlider);
    
    if (isSliderDisabled(progressSlider)) {
        DrawRectangleRounded(rect, getRoundnessForRoundedRectangle(rect, visualizerRectRadius-interfaceSpace1), 4, blendColors(visualizerBackgroundColor, BLACK, 0.2));
        float textSize = 0.032*floatMin(screenSize.x, screenSize.y);
        renderFontStringAlign(GlobalFonts[0].font, "No Audio Available", getRectangleCenter(rect), (Vector2){0.5,0.5}, textSize, 0, COLOR_TEXT_4);
    } else {
        float val = sliderGetSlideValue(progressSlider);
        Rectangle trect = {rect.x, rect.y, rect.width*val, rect.height};
        DrawRectangleRounded(trect, getRoundnessForRoundedRectangle(trect, visualizerRectRadius-interfaceSpace1), 4, blendColors(_getCol(val*7.0), WHITE, 0.2*effect));
    }
}

static void _renderVisualizerControl() {
    DrawRectangleRounded(controlRect, getRoundnessForRoundedRectangle(controlRect, visualizerRectRadius), 4, visualizerBackgroundColor);

    float textSize = 0.04*floatMin(screenSize.x, screenSize.y);
    const char* title = projectGetCurrentTitle();
    if (!title || strlen(title)==0) title = PROJECT_TITLE_PLACEHOLDER;

    renderFontStringAlign(GlobalFonts[0].font, title, getRectangleCenter(controlRect), (Vector2){0.5, 0.5}, textSize, 0, COLOR_PALETTE_1_P9);
}

void renderVisualizer() {
    _renderBackButton();

    _renderFFTVisualizer();
    
    _renderVisualizerProgress();
    _renderVisualizerControl();
    _renderVisualizerDecorations();
}
