#include <interface.h>
#include <backend.h>
#include <ui.h>
#include <colors.h>
#include <utils.h>
#include <visualizer.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <synth.h>
#include <rlgl.h>
#include <raymath.h>
#include <export.h>
#include <threads.h>
#include <tinyfiledialogs.h>


#define PROJECT_TITLE_PLACEHOLDER "Project Title"
#define M_TAU 6.28318530717958647


static Button backButton=NULL, playPauseButton=NULL, restartButton=NULL, nextVisualizationButton=NULL, recordButton=NULL;
static Slider progressSlider=NULL;
static Rectangle visualizerRect={0,0,0,0}, progressRect={0,0,0,0}, controlRect={0,0,0,0}, renderTypeRect={0,0,0,0}, controlDurationRect={0,0,0,0}, controlTitleRect={0,0,0,0}, wholeDivRect={0,0,0,0};
static float visualizerRectRadius=0, visualizerRectRoundness=0.08, maxIntensity=0;
static struct duration_data duration={0,0};
static enum visualizer_type visualizerType = VISUALIZER_TYPE_1;
static Color visualizerBackgroundColor={0,0,0,0}, disabledBackgroundColor={0,0,0,0}, textOrHoverBackgroundColor={0,0,0,0};
static int progressWasHolding=0, progressWasPlaying=0, ffmpegAvailable=0;

static Vector2 expActuaVideoSize={1920,1080}, expTextureSize={1920,1080};
static int expTextureScale=1, expVideoActive=0;
static RenderTexture2D expTexture={0,};
static Rectangle expVisualizerRect={0,0,0,0}, expProgressRect={0,0,0,0}, expControlRect={0,0,0,0}, expRenderTypeRect={0,0,0,0}, expControlDurationRect={0,0,0,0}, expControlTitleRect={0,0,0,0}, expWholeDivRect={0,0,0,0};
static Rectangle expBtnRestartRect={0,0,0,0}, expBtnPlayRect={0,0,0,0}, expBtnNextRect={0,0,0,0}, expSldProgressRect={0,0,0,0};
static float expVisualizerRectRadius=0, expVisualizerRectRoundness=0.08, expInterfaceSpace1=0, expControlLineHeight=0, expInterfaceSpace2=0, expTextSize=0, expSimulationTime=0, expSimulationTotalDuration=0, expMaxIntensity=0, expAudioDuration=0;


void visualizerInit() {
    Rectangle rect = {0,0,20,20};

    if (backButton) buttonFree(backButton);
    backButton = buttonCreate(rect, 0.25);

    if (playPauseButton) buttonFree(playPauseButton);
    playPauseButton = buttonCreate(rect, 0.25);

    if (restartButton) buttonFree(restartButton);
    restartButton = buttonCreate(rect, 0.25);

    if (nextVisualizationButton) buttonFree(nextVisualizationButton);
    nextVisualizationButton = buttonCreate(rect, 0.25);

    ffmpegAvailable = isFFmpegAvailable();
    if (recordButton) buttonFree(recordButton);
    recordButton = ffmpegAvailable?buttonCreate(rect, 0.25):NULL;

    if (progressSlider) sliderFree(progressSlider);
    progressSlider = sliderCreate(rect, 0.5);

    duration = projectGetDuration();
}



void visualizerClose() {
    if (backButton) buttonFree(backButton);
    backButton=NULL;
    if (playPauseButton) buttonFree(playPauseButton);
    playPauseButton=NULL;
    if (restartButton) buttonFree(restartButton);
    restartButton=NULL;
    if (nextVisualizationButton) buttonFree(nextVisualizationButton);
    nextVisualizationButton=NULL;
    if (recordButton) buttonFree(recordButton);
    recordButton=NULL;
    if (progressSlider) sliderFree(progressSlider);
    progressSlider=NULL;
    ffmpegAvailable=0;
}

static void _visualizationSimUpdateOnce() {
    float tl = expTextureSize.y;
    if (expTextureSize.y/expTextureSize.x>0.666) tl = expTextureSize.x*0.666;
    expControlLineHeight = floatMin(0.12*tl, 100);

    expInterfaceSpace1 = 0.1*expControlLineHeight;
    expInterfaceSpace2 = 0.4*expControlLineHeight;

    expTextSize = 0.04*floatMin(expTextureSize.x, expTextureSize.y);

    float tx = 0.05*floatMin(expTextureSize.x, expTextureSize.y);
    
    float tmpVisRectH = expTextureSize.y-2.85*expControlLineHeight-2*expInterfaceSpace1-tx;
    float totalH = expTextureSize.y-tx;//(tmpVisRectH-tx+2.85*expControlLineHeight+2*expInterfaceSpace1);
    float sty = expTextureSize.y*0.5+0.5*totalH-tmpVisRectH;
    expVisualizerRect = (Rectangle){0.5*tx, sty, expTextureSize.x-tx, tmpVisRectH};
    expVisualizerRectRadius = getRadiusForRoundedRectangle(expVisualizerRect, expVisualizerRectRoundness);

    expProgressRect = (Rectangle){expVisualizerRect.x, expVisualizerRect.y-0.85*expControlLineHeight, expVisualizerRect.width, 0.85*expControlLineHeight-expInterfaceSpace1};
    expControlRect = (Rectangle){expVisualizerRect.x, expProgressRect.y-expControlLineHeight-expInterfaceSpace1, expVisualizerRect.width, expControlLineHeight};
    expRenderTypeRect = (Rectangle){expVisualizerRect.x, expControlRect.y-expControlLineHeight-expInterfaceSpace1, expVisualizerRect.width, expControlLineHeight};
    expWholeDivRect = (Rectangle){0, 0, expTextureSize.x, expTextureSize.y};


    float tknprc = 0.75;    
    expSldProgressRect = (Rectangle){expProgressRect.x+expInterfaceSpace1, expProgressRect.y+expInterfaceSpace1, expProgressRect.width-2*expInterfaceSpace1, expProgressRect.height-2*expInterfaceSpace1};
    expSimulationTime = exportVideoSimulationGetTime();
    expSimulationTotalDuration = exportVideoSimulationGetDuration();
    

    Rectangle* rects[3] = {&expBtnRestartRect, &expBtnPlayRect, &expBtnNextRect};
    float x=expControlRect.x+0.5*(1.0-tknprc)*expControlRect.height, y=expControlRect.y+0.5*(1.0-tknprc)*expControlRect.height, w=tknprc*expControlRect.height, offset=tknprc*expControlRect.height+expInterfaceSpace1;
    for (int i=0; i<3; i++) {
        Rectangle rect = {x, y, w, w};
        *(rects[i]) = rect;        
        x += offset;
    }

    x+=expInterfaceSpace2-expInterfaceSpace1;
    expControlDurationRect = (Rectangle){x, y, 3*w, w};
    x+=expControlDurationRect.width+expInterfaceSpace2;
    expControlTitleRect = (Rectangle){x, y, expControlRect.x+expControlRect.width-x-0.5*(1.0-tknprc)*expControlRect.height, w};

    expMaxIntensity=0;
    expAudioDuration = exportVideoSimulationGetAudioDuration();
}

void visualizationRenderSimClose() {
    if (expTexture.id) UnloadRenderTexture(expTexture);
    expTexture=(RenderTexture2D){0,};
    expVideoActive = 0;
}

void visualizationRenderSimInit() {
    visualizationRenderSimClose();
    expVideoActive = 1;
    expTextureSize = Vector2Scale(expActuaVideoSize, expTextureScale);
    expTexture=LoadRenderTexture((int)(expTextureSize.x), (int)(expTextureSize.y));
    if (!(expTexture.id)) {
        visualizationRenderSimClose();
        return;
    }
    //SetTextureFilter(expTexture.texture, TEXTURE_FILTER_BILINEAR);
    _visualizationSimUpdateOnce();
}



static void _backToRegularRender() {
    visualizerClose();
    globalHandlerSetRenderType(ART_REGULAR);
    expVideoActive = 0;
}

struct duration_data visualizerGetDurationData() {
    if (backButton) return duration;
    return (struct duration_data){0,0};
}

static void _actionTogglePausePlay() {
    if (!(duration.timestamp)) return;

    int isPlaying = globalHandlerIsPlaying();
    uint32_t timestamp = globalHandlerGetLineTimestamp();

    if (isPlaying) {
        synthPanic();
        globalHandlerPause();
    } else if (timestamp<duration.timestamp-10) globalHandlerPlay();
    else {
        globalHandlerSetLineTime(0);
        globalHandlerPlay();
    }
}

static void _actionBackToTheStart() {
    if (!(duration.timestamp)) return;
    int isPlaying = globalHandlerIsPlaying();
    if (isPlaying) {
        synthPanic();
        globalHandlerPause();
    }
    globalHandlerSetLineTime(0);
}

static void _actionNextVisualization() {
    globalHandlerToggleNextVisualization();
}

static void _actionLaunchVideoSubprocess() {
    globalHandlerPause();
    synthPanic();

    const char* projectTitle = projectGetCurrentTitle();
    if (!projectTitle) projectTitle = PROJECT_TITLE_PLACEHOLDER;

    char* title = stringToFileName(projectTitle, 30);
    char* conct = concatenateStrings(title, ".mp4");
    free(title);

    const char* path = tinyfd_saveFileDialog("Export Visualization", conct, 1, (const char *[]){"*.mp4"}, "MP4 Format");
    free(conct);

    if (path) {
        expVideoActive = 1;
        threadRequestExportVideoFFTVisualization(path);
    }
}


static void _updateVisualizerButtons() {
    float tknprc = 0.75;

    if (backButton) {
        //Rectangle rect = {0.2*l, 0.2*l, 0.8*l, 0.8*l};
        float x=renderTypeRect.x+0.5*(1.0-tknprc)*renderTypeRect.height, y=renderTypeRect.y+0.5*(1.0-tknprc)*renderTypeRect.height, w=tknprc*renderTypeRect.height;
        Rectangle rect = {x, y, w, w};
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

    Button btns[] = {restartButton, playPauseButton, nextVisualizationButton, recordButton};
    OnClickFunc actions[] = {_actionBackToTheStart, _actionTogglePausePlay, _actionNextVisualization, _actionLaunchVideoSubprocess};

    int s = sizeof(btns)/sizeof(Button);
    float x=controlRect.x+0.5*(1.0-tknprc)*controlRect.height, y=controlRect.y+0.5*(1.0-tknprc)*controlRect.height, w=tknprc*controlRect.height, offset=tknprc*controlRect.height+interfaceSpace1;
    for (int i=0; i<s; i++) {
        Button btn = btns[i];
        if (btn) {
            Rectangle rect = {x, y, w, w};
            buttonUpdateRectangle(btn, rect);
            if (duration.timestamp) buttonEnable(btn);
            else buttonDisable(btn);
            buttonUpdate(btn, -1);
            if (actions[i] && (isButtonClicked(btn) || (btn==playPauseButton && IsKeyPressed(KEY_SPACE)))) actionDefer(actions[i]);
            x += offset;
        }
    }

    x+=interfaceSpace2-interfaceSpace1;
    controlDurationRect = (Rectangle){x, y, 3*w, w};
    x+=controlDurationRect.width+interfaceSpace2;
    controlTitleRect = (Rectangle){x, y, controlRect.x+controlRect.width-x-0.5*(1.0-tknprc)*controlRect.height, w};

}

static void _updateVisualizerValues() {
    float tx = 0.05*floatMin(screenSize.x, screenSize.y);
    
    float totalH = (0.6*screenSize.y-tx+2.85*controlLineHeight+2*interfaceSpace1);
    float sty = screenSize.y*0.5+0.5*totalH-(screenSize.y*0.6-tx);
    visualizerRect = (Rectangle){tx, sty, screenSize.x-2*tx, screenSize.y*0.6-tx};         // screenSize.y*0.4
    visualizerRectRadius = getRadiusForRoundedRectangle(visualizerRect, visualizerRectRoundness);

    visualizerBackgroundColor = blendColors(COLOR_BACKGROUND_3, COLOR_BACKGROUND_1, 0.3);
    disabledBackgroundColor = blendColors(visualizerBackgroundColor, BLACK, 0.2);
    textOrHoverBackgroundColor = blendColors(visualizerBackgroundColor, WHITE, 0.03);

    progressRect = (Rectangle){visualizerRect.x, visualizerRect.y-0.85*controlLineHeight, visualizerRect.width, 0.85*controlLineHeight-interfaceSpace1};

    controlRect = (Rectangle){visualizerRect.x, progressRect.y-controlLineHeight-interfaceSpace1, visualizerRect.width, controlLineHeight};
    renderTypeRect = (Rectangle){visualizerRect.x, controlRect.y-controlLineHeight-interfaceSpace1, visualizerRect.width, controlLineHeight};

    wholeDivRect = (Rectangle){0.5*tx, (screenSize.y-totalH-tx)*0.5, screenSize.x-tx, totalH+tx};
}


void order2PrecomputeVisualizer() {
    if (globalHandlerGetRenderType()!=ART_VISUALIZER || expVideoActive) return;

    _updateVisualizerValues();
    _updateVisualizerButtons();
    
    FFTupdate(&maxIntensity);
}


void visualizationRenderSimPrecomputeValues(float maxIntensity) {
    expSimulationTime = exportVideoSimulationGetTime();
    expMaxIntensity = maxIntensity;
}


static void _renderBackButton() {
    if (!backButton) return;

    float effect = buttonGetEffectValue(backButton);
    Color col1=textOrHoverBackgroundColor, col2={32, 34, 40, 255};
    Color blendedCol = blendColors(col1, col2, effect);

    Rectangle rect = buttonGetRectangle(backButton);
    Rectangle trect = scaleRctangleFromCenter(rect, lerp(0.3, 1, effect));
    DrawRectangleRounded(trect, getRoundnessForRoundedRectangle(trect, visualizerRectRadius+0.5*(trect.height-renderTypeRect.height)), 4, blendedCol);
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

static void _renderVisualizerVerticalLineAtFrequency(float freq, float maxFreq, int export, float thickness) {
    Rectangle vrect = export?expVisualizerRect:visualizerRect;
    float vrad = export?expVisualizerRectRadius:visualizerRectRadius;

    float start=vrect.x+vrad, end=vrect.x+vrect.width-vrad;
    float x = lerp(start, end, freq/maxFreq), y=vrect.y+vrad, h=vrect.height-2*vrad, w=1.0;
    DrawRectangleRec((Rectangle){x,y,w*thickness,h}, (Color){150,150,150,20*thickness});
}

static float _midiGetFreq(float note) {
    return 440.0*pow(2.0, (note-69.0)/12.0);
}



static void _visualizer1(int num, float* buffer, int export) {
    int steps = 2;
    int iters = num/steps;
    float maxFreq = FFTgetDeltaFrequency()*(iters-1);
    int semitones[7]={0,2,4,5,7,9,11};
    for (int i=0; i<10; i++) {
        for (int j=0; j<7; j++) {
            float freq = _midiGetFreq(12+12*i+semitones[j]);
            float thickness = export?(1.0+1.0*(j==0)):(0.7+0.3*(j==0));
            if (freq<=maxFreq) _renderVisualizerVerticalLineAtFrequency(freq, maxFreq, export, thickness);
        }
    }
    
    Rectangle vrect = export?expVisualizerRect:visualizerRect;
    float vrad = export?expVisualizerRectRadius:visualizerRectRadius;

    float w=(vrect.width-2*vrad)/iters, x=vrect.x+vrad, y=vrect.y+vrect.height-vrad, h=vrect.height-2*vrad;
    float w2=w*0.5;
    float lg2b10 = 1.0/log10(2.0);

    for (int i=0; i<iters; i++) {
        float v = lg2b10*log10(floatClip(sqrt(buffer[i]*23.5/num), 0.0, 1.0)+1.0);
        DrawRectangleRec((Rectangle){x+0.5*w2, y-h*v, w2, h*v}, _getCol(7.0*v));
        x += w;
    }
}

static void _visualizer2(int num, float* buffer, int export) {
    int steps = 2;
    int iters = num/steps;
    float maxFreq = FFTgetDeltaFrequency()*(iters-1);
    for (int i=0; i<10; i++) {
        float freq = _midiGetFreq(12+12*i);
        if (freq<=maxFreq) _renderVisualizerVerticalLineAtFrequency(freq, maxFreq, export, 1.0);
    }

    Rectangle vrect = export?expVisualizerRect:visualizerRect;
    float vrad = export?expVisualizerRectRadius:visualizerRectRadius;

    float w=(vrect.width-2*vrad)/iters, x=vrect.x+vrad, y=vrect.y+vrect.height-vrad, h=vrect.height-2*vrad;
    float lg2b10 = 1.0/log10(2.0);

    for (int i=0; i<iters; i++) {
        float v = lg2b10*log10(floatClip(sqrt(buffer[i]*23.5/num), 0.0, 1.0)+1.0);
        DrawTriangle((Vector2){x,y}, (Vector2){x+w,y}, (Vector2){x+0.5*w, y-h*v}, _getCol(7.0*v));      
        x += w;
    }
}

static void _visualizer3(int num, float* buffer, int export) {
    int iters = num>>3;
    
    Rectangle vrect = export?expVisualizerRect:visualizerRect;
    float vrad = export?expVisualizerRectRadius:visualizerRectRadius;

    float w=(vrect.width-2*vrad)/iters, y=vrect.y+vrect.height-vrad, h=vrect.height-2*vrad;
    float w2=w*0.8;
    float lg2b10 = 1.0/log10(2.0), x=vrect.x+vrad+(w-w2)*0.5;

    for (int i=0; i<iters; i++) {
        float v = lg2b10*log10(floatClip(sqrt(buffer[i]*23.5/num), 0.0, 1.0)+1.0);
        float height = w*floor((v*h)/w);
        Color c1 = _getCol(7.0*v);
        DrawRectangleGradientEx((Rectangle){x, y-height, w2, height}, c1, COLOR_TRACK_THEME_0, COLOR_TRACK_THEME_0, c1);
        x += w;
    }
    
    float w3=vrect.width-vrad, w4=w-w2;
    x=vrect.x+0.5*vrad, y-=0.5*w4;
    while (y>vrect.y+vrad) {
        DrawRectangleRec((Rectangle){x, y, w3, w4}, visualizerBackgroundColor);
        y-=w;
    }
}

static void _visualizer4(int num, float* buffer, int export) {
    int iters = num>>3;

    Rectangle vrect = export?expVisualizerRect:visualizerRect;
    float vrad = export?expVisualizerRectRadius:visualizerRectRadius;

    int totalStringPieces=vrect.width*0.2;
    float x=vrect.x+vrad, cy=vrect.y+0.5*vrect.height, factor=0.9*vrect.height/num, w=(vrect.width-2*vrad)/totalStringPieces;
    float thickness=floatMax(1.0, vrect.width*0.00315), minY=vrect.y+vrad, maxY=vrect.y+vrect.height-vrad, h=0.5*vrect.height-vrad, jfactor=2.0*M_TAU/iters;
    Vector2 oldPos={0,0}, newPos={x,cy};
    
    float time = export?expSimulationTime:globalHandlerGetLineTime();
    float ranPhase = fmod(time*0.4, M_TAU);
    float startR = 0.0;
    for (int j=1; j<iters; j++) startR += buffer[j]*sin(ranPhase*j+j*jfactor);
    startR = factor*startR;
    float startY=floatClip(cy-startR, minY, maxY);
    newPos = (Vector2){x, startY};
    Color startCol =  _getCol(7.0*fabs((startY-cy)/h));
    DrawCircleV(newPos, 0.5*thickness, startCol);
    x += w;

    for (int i=1; i<totalStringPieces; i++) {
        float phase=ranPhase+(M_TAU*i)/totalStringPieces;
        float r = 0.0;
        for (int j=1; j<iters; j++) r += buffer[j]*sin(phase*j+j*jfactor);
        r = factor*r;
        oldPos=newPos;
        newPos = (Vector2){x, floatClip(cy-r, minY, maxY)};
        Color col = _getCol(7.0*fabs((newPos.y-cy)/h));
        DrawLineEx(oldPos, newPos, thickness, col);
        DrawCircleV(newPos, 0.5*thickness, col);
        x += w;
    }
    oldPos = (Vector2){x,startY};
    DrawLineEx(oldPos, newPos, thickness, startCol);
    DrawCircleV(oldPos, 0.5*thickness, startCol);
}

static void _renderFFTVisualizer() {
    DrawRectangleRounded(visualizerRect, visualizerRectRoundness, 4, visualizerBackgroundColor);

    int num=0;
    float* buffer = FFTgetIntensityBuffer(&num);
    if (num<=0) return;

    visualizerType = globalHandlerGetVisualizerType();

    switch (visualizerType) {
        case VISUALIZER_TYPE_1: {
            _visualizer1(num, buffer, 0);
            return;
        }
        case VISUALIZER_TYPE_2: {
            _visualizer2(num, buffer, 0);
            return;
        }
        case VISUALIZER_TYPE_3: {
            _visualizer3(num, buffer, 0);
            return;
        }
        case VISUALIZER_TYPE_4: {
            _visualizer4(num, buffer, 0);
            return;
        }
        default: return;
    }
    
}

static void _renderBackground() {
    float tx = 0.05*floatMin(screenSize.x, screenSize.y);

    float v = 7.0*log10(floatClip(sqrt(maxIntensity*51.2/FFTgetBufferLength()), 0.0, 1.0)+1.0)/log10(2.0);
    Color col = _getCol(v);
    DrawRectangleRounded(wholeDivRect, getRoundnessForRoundedRectangle(wholeDivRect, visualizerRectRadius+0.5*tx), 8, blendColors(visualizerBackgroundColor, col, 0.1+0.04*v));
    Rectangle innerRect = rectangleIncrease(wholeDivRect, (Vector2){-0.25*tx, -0.25*tx}, (Vector2){-0.25*tx, -0.25*tx});
    DrawRectangleRounded(innerRect, getRoundnessForRoundedRectangle(innerRect, visualizerRectRadius+0.25*tx), 8, COLOR_BACKGROUND_1);

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

static void _renderVisualizerDecorations() {
    DrawRectangleRounded(renderTypeRect, getRoundnessForRoundedRectangle(renderTypeRect, visualizerRectRadius), 4, visualizerBackgroundColor);
    const char* text = "FFT Spectrum Visualizer";
    float textSize = 0.04*floatMin(screenSize.x, screenSize.y);
    float finalSize = _getTextSizeToFitInRect(renderTypeRect, textSize, text, interfaceSpace2);
    renderFontStringAlign(GlobalFonts[0].font, text, getRectangleCenter(renderTypeRect), (Vector2){0.5, 0.5}, finalSize, 0, COLOR_TEXT_1);

    _renderBackButton();
}


static void _renderVisualizerProgress() {
    if (!progressSlider) return;

    float effect = sliderGetEffectValue(progressSlider);
    Color bkgCol = blendColors(visualizerBackgroundColor, COLOR_BACKGROUND_3, effect*0.8);
    DrawRectangleRounded(progressRect, getRoundnessForRoundedRectangle(progressRect, visualizerRectRadius), 4, bkgCol);
    Rectangle rect = sliderGetRectangle(progressSlider);
    
    if (isSliderDisabled(progressSlider)) {
        DrawRectangleRounded(rect, getRoundnessForRoundedRectangle(rect, visualizerRectRadius-interfaceSpace1), 4, disabledBackgroundColor);
        float textSize = 0.032*floatMin(screenSize.x, screenSize.y);
        renderFontStringAlign(GlobalFonts[0].font, "No Audio Available", getRectangleCenter(rect), (Vector2){0.5,0.5}, textSize, 0, COLOR_TEXT_4);
    } else {
        float val = sliderGetSlideValue(progressSlider);
        if (val>0) {
            float radius = visualizerRectRadius-interfaceSpace1;
            float vw = rect.width*val, invw = rect.width*(1.0-val);
            Rectangle lrect = {rect.x, rect.y, 2*radius, rect.height};
            Rectangle rrect = {rect.x+rect.width-2*radius, rect.y, 2*radius, rect.height};
            Rectangle nrect = {rect.x+radius, rect.y, floatMin(vw-radius, rect.width-2*radius), rect.height};

            Color col = blendColors(_getCol(val*7.0), WHITE, 0.2*effect);
            
            DrawRectangleRounded(lrect, 1.0, 4, col);
            if (vw<2*radius) {
                Rectangle srect = {rect.x+vw, rect.y, rect.width*(0.5-val), rect.height};
                DrawRectangleRec(srect, bkgCol);
            }
            if (vw>radius) DrawRectangleRec(nrect, col);
            if (invw<radius) {
                DrawRectangleRounded(rrect, 1.0, 4, col);
                Rectangle srect = {rect.x+vw, rect.y, invw, rect.height};
                DrawRectangleRec(srect, bkgCol);
            }
        }
    }
}

static char* _generateTimestampString(double curT, double durT) {
    curT = floatMin(curT, durT);
    uint32_t totalSeconds=floor(durT), currentSecond=floor(curT);

    if (totalSeconds<60*60) {
        return strdup(TextFormat("%u:%02u / %u:%02u", currentSecond/60, currentSecond%60, totalSeconds/60, totalSeconds%60));
    } else {
        if (currentSecond<60*60) return strdup(TextFormat("0:%02u:%02u / %u:%02u:%02u", currentSecond/60, currentSecond%60, totalSeconds/(60*60), (totalSeconds-60*60*(totalSeconds/(60*60)))/60, totalSeconds%60));
        else return strdup(TextFormat("%u:%02u:%02u / %u:%02u:%02u", currentSecond/(60*60), (currentSecond-(60*60*(currentSecond/(60*60))))/60, currentSecond%60, totalSeconds/(60*60), (totalSeconds-60*60*(totalSeconds/(60*60)))/60, totalSeconds%60));
    }
}

static char* generateTimestampString() {
    if (!duration.timestamp) return strdup("-:-- / -:--");
    double curT = floatMin(globalHandlerGetLineTime(), duration.time);
    return _generateTimestampString(curT, duration.time);
}

static void _renderVerticalSeperator(float x, float ty, float h, float screenX) {
    Color col = COLOR_TEXT_4; col.a=50;
    DrawLineEx((Vector2){x, ty+0.2*h}, (Vector2){x, ty+0.8*h}, floatMax(1, screenX*0.002), col);
}

static void _renderVisualizerControl() {
    DrawRectangleRounded(controlRect, getRoundnessForRoundedRectangle(controlRect, visualizerRectRadius), 4, visualizerBackgroundColor);

    float knownRoundness = getRoundnessForRoundedRectangle(controlTitleRect, visualizerRectRadius+0.5*(controlTitleRect.height-controlRect.height));
    float textSize = 0.04*floatMin(screenSize.x, screenSize.y);


    int isPlaying = globalHandlerIsPlaying();
    Button btns[] = {restartButton, playPauseButton, nextVisualizationButton, recordButton};
    enum icon_title btnIcons[] = {T_ICON_PREVIOUS, isPlaying?T_ICON_PAUSE:T_ICON_PLAY, T_ICON_EFFECTS, T_ICON_VIDEO};
    float sizes[] = {0.85, isPlaying?0.8:0.65, 1.05, 0.92};
    Color col1=textOrHoverBackgroundColor, col2={32,34,40,255}, col3={45,32,34,255};
    Color cols[]={col2, col2, col2, col3}, colsFg[]={COLOR_PALETTE_1_P9, COLOR_PALETTE_1_P9, COLOR_PALETTE_1_P9, (Color){255, 180, 192, 255}};
    int s = sizeof(btns)/sizeof(Button);
    

    for (int i=0; i<s; i++) {
        Button btn = btns[i];
        if (btn) {
            Rectangle rect = buttonGetRectangle(btn);
            float effect = buttonGetEffectValue(btn);
            Rectangle trect = scaleRctangleFromCenter(rect, lerp(0.3, 1, effect));
            int enabled = isButtonEnabled(btn);

            Color tmp1=col1, tmp2=cols[i];
            if (!enabled) tmp1=disabledBackgroundColor;

            Color blendedCol = blendColors(tmp1, tmp2, effect);
            
            DrawRectangleRounded(trect, getRoundnessForRoundedRectangle(trect, visualizerRectRadius+0.5*(trect.height-controlRect.height)), 4, blendedCol);
            iconRerder(btnIcons[i], scaleRctangleFromCenter(rect, 0.95*sizes[i]*lerp(0.9, 0.95, effect)), blendColors(enabled?COLOR_TEXT_1:COLOR_TEXT_4, colsFg[i], effect));
        }
    }



    
    const char* title = projectGetCurrentTitle();
    if (!title || strlen(title)==0) title = PROJECT_TITLE_PLACEHOLDER;

    DrawRectangleRounded(controlTitleRect, knownRoundness, 4, textOrHoverBackgroundColor);
    
    float finalSize = _getTextSizeToFitInRect(controlTitleRect, textSize, title, interfaceSpace2);
    renderFontStringAlign(GlobalFonts[0].font, title, getRectangleCenter(controlTitleRect), (Vector2){0.5, 0.5}, finalSize, 0, COLOR_PALETTE_1_P9);

    DrawRectangleRounded(controlDurationRect, knownRoundness, 4, duration.timestamp?textOrHoverBackgroundColor:disabledBackgroundColor);
    char* dur = generateTimestampString();
    const char* tmp = (dur)?dur:"-:-- / -:--";
    finalSize = _getTextSizeToFitInRect(controlDurationRect, textSize, tmp, interfaceSpace2);
    renderFontStringAlign(GlobalFonts[0].font, tmp, getRectangleCenter(controlDurationRect), (Vector2){0.5, 0.5}, finalSize, 0, COLOR_PALETTE_1_P9);
    if (dur) {
        free(dur);
        dur=NULL;
    }
    tmp=NULL;

    _renderVerticalSeperator(controlDurationRect.x-0.5*interfaceSpace2, controlRect.y, controlRect.height, screenSize.x);
    _renderVerticalSeperator(controlDurationRect.x+controlDurationRect.width+0.5*interfaceSpace2, controlRect.y, controlRect.height, screenSize.x);


}

void _renderWaitPlaceholder() {
    float textSize = 0.5*floatMin(screenSize.x, screenSize.y*0.1);
    renderFontStringAlign(GlobalFonts[0].font, "Rendering Video...", Vector2Scale(screenSize, 0.5), (Vector2){0.5,0.5}, textSize, 0, COLOR_TEXT_3);
}

void renderVisualizer() {
    if (expVideoActive) {
        //_renderWaitPlaceholder();
        return;
    } else {
        _renderBackground();
        _renderFFTVisualizer();
        _renderVisualizerProgress();
        _renderVisualizerControl();
        _renderVisualizerDecorations();
    }
}



static void _expRenderBackground() {
    ClearBackground(COLOR_BACKGROUND_1);

    float tx = 0.05*floatMin(expTextureSize.x, expTextureSize.y);
    float v = 7.0*log10(floatClip(sqrt(expMaxIntensity*51.2/FFTgetBufferLength()), 0.0, 1.0)+1.0)/log10(2.0);
    Color col = _getCol(v);
    DrawRectangleRounded(expWholeDivRect, getRoundnessForRoundedRectangle(expWholeDivRect, expVisualizerRectRadius+0.5*tx), 8, blendColors(visualizerBackgroundColor, col, 0.1+0.04*v));
    Rectangle innerRect = rectangleIncrease(expWholeDivRect, (Vector2){-0.25*tx, -0.25*tx}, (Vector2){-0.25*tx, -0.25*tx});
    DrawRectangleRounded(innerRect, getRoundnessForRoundedRectangle(innerRect, expVisualizerRectRadius+0.25*tx), 8, COLOR_BACKGROUND_1);
}

static void _expRenderFFTVisualizer() {
    DrawRectangleRounded(expVisualizerRect, expVisualizerRectRoundness, 8, visualizerBackgroundColor);
    int num=0;
    float* buffer = FFTexportGetIntensityBuffer(&num);
    if (num<=0) return;

    switch (visualizerType) {
        case VISUALIZER_TYPE_1: {
            _visualizer1(num, buffer, 1);
            return;
        }
        case VISUALIZER_TYPE_2: {
            _visualizer2(num, buffer, 1);
            return;
        }
        case VISUALIZER_TYPE_3: {
            _visualizer3(num, buffer, 1);
            return;
        }
        case VISUALIZER_TYPE_4: {
            _visualizer4(num, buffer, 1);
            return;
        }
        default: return;
    }
}

static void _expRenderVisualizerProgress() {
    Color bkgCol = visualizerBackgroundColor;
    DrawRectangleRounded(expProgressRect, getRoundnessForRoundedRectangle(expProgressRect, expVisualizerRectRadius), 8, bkgCol);
    Rectangle rect = expSldProgressRect;
    
    float val = floatClip(expSimulationTime/expAudioDuration, 0.0, 1.0);
    if (val>0) {
        float radius = expVisualizerRectRadius-expInterfaceSpace1;
        float vw = rect.width*val, invw = rect.width*(1.0-val);
        Rectangle lrect = {rect.x, rect.y, 2*radius, rect.height};
        Rectangle rrect = {rect.x+rect.width-2*radius, rect.y, 2*radius, rect.height};
        Rectangle nrect = {rect.x+radius, rect.y, floatMin(vw-radius, rect.width-2*radius), rect.height};

        Color col = _getCol(val*7.0);
        
        DrawRectangleRounded(lrect, 1.0, 4, col);
        if (vw<2*radius) {
            Rectangle srect = {rect.x+vw, rect.y, rect.width*(0.5-val), rect.height};
            DrawRectangleRec(srect, bkgCol);
        }
        if (vw>radius) DrawRectangleRec(nrect, col);
        if (invw<radius) {
            DrawRectangleRounded(rrect, 1.0, 4, col);
            Rectangle srect = {rect.x+vw, rect.y, invw, rect.height};
            DrawRectangleRec(srect, bkgCol);
        }
    }
}

static void _expRenderVisualizerControl() {
    DrawRectangleRounded(expControlRect, getRoundnessForRoundedRectangle(expControlRect, expVisualizerRectRadius), 8, visualizerBackgroundColor);

    float knownRoundness = getRoundnessForRoundedRectangle(expControlTitleRect, expVisualizerRectRadius+0.5*(expControlTitleRect.height-expControlRect.height));

    int isPlaying = (expSimulationTime<expAudioDuration);
    Rectangle* rects[3] = {&expBtnRestartRect, &expBtnPlayRect, &expBtnNextRect};
    enum icon_title btnIcons[] = {T_ICON_PREVIOUS, isPlaying?T_ICON_PAUSE:T_ICON_PLAY, T_ICON_EFFECTS, T_ICON_VIDEO};
    float sizes[] = {0.85, isPlaying?0.8:0.65, 1.05, 0.92};
    Color col1=textOrHoverBackgroundColor;
    int s = sizeof(rects)/sizeof(Rectangle*);
    

    for (int i=0; i<s; i++) {
        Rectangle rect = *rects[i];
        Rectangle trect = scaleRctangleFromCenter(rect, 0.3);

        Color blendedCol = col1;
        
        DrawRectangleRounded(trect, getRoundnessForRoundedRectangle(trect, expVisualizerRectRadius+0.5*(trect.height-expControlRect.height)), 4, blendedCol);
        iconRerder(btnIcons[i], scaleRctangleFromCenter(rect, 0.95*sizes[i]*0.9), COLOR_TEXT_1);
    }


    const char* title = projectGetCurrentTitle();
    if (!title || strlen(title)==0) title = PROJECT_TITLE_PLACEHOLDER;

    DrawRectangleRounded(expControlTitleRect, knownRoundness, 8, textOrHoverBackgroundColor);
    
    float finalSize = _getTextSizeToFitInRect(expControlTitleRect, expTextSize, title, expInterfaceSpace2);
    renderFontStringAlign(GlobalFonts[0].font, title, getRectangleCenter(expControlTitleRect), (Vector2){0.5, 0.5}, finalSize, 0, COLOR_PALETTE_1_P9);

    DrawRectangleRounded(expControlDurationRect, knownRoundness, 4, textOrHoverBackgroundColor);
    char* dur = _generateTimestampString(floatClip(expSimulationTime, 0.0, expAudioDuration), expAudioDuration);
    const char* tmp = (dur)?dur:"-:-- / -:--";
    finalSize = _getTextSizeToFitInRect(expControlDurationRect, expTextSize, tmp, expInterfaceSpace2);
    renderFontStringAlign(GlobalFonts[0].font, tmp, getRectangleCenter(expControlDurationRect), (Vector2){0.5, 0.5}, finalSize, 0, COLOR_PALETTE_1_P9);
    if (dur) {
        free(dur);
        dur=NULL;
    }
    tmp=NULL;

    _renderVerticalSeperator(expControlDurationRect.x-0.5*expInterfaceSpace2, expControlRect.y, expControlRect.height, expTextureSize.x);
    _renderVerticalSeperator(expControlDurationRect.x+expControlDurationRect.width+0.5*expInterfaceSpace2, expControlRect.y, expControlRect.height, expTextureSize.x);
}

static void _expRrenderVisualizerDecorations() {
    DrawRectangleRounded(expRenderTypeRect, getRoundnessForRoundedRectangle(expRenderTypeRect, expVisualizerRectRadius), 8, visualizerBackgroundColor);
    const char* text = "FFT Spectrum Visualizer";
    float finalSize = _getTextSizeToFitInRect(expRenderTypeRect, expTextSize, text, expInterfaceSpace2);
    renderFontStringAlign(GlobalFonts[0].font, text, getRectangleCenter(expRenderTypeRect), (Vector2){0.5, 0.5}, finalSize, 0, COLOR_TEXT_1);
}

void visualizationRenderSimRender() {
    if (!expTexture.id) return;

    BeginTextureMode(expTexture);
        _expRenderBackground();
        _expRenderFFTVisualizer();
        _expRenderVisualizerProgress();
        _expRenderVisualizerControl();
        _expRrenderVisualizerDecorations();
    EndTextureMode();
}

Image visualizationSimGetImage() {
    if (!expTexture.id) return (Image){NULL,};

    //Image highResImg = LoadImageFromTexture(expTexture.texture);
    //ImageResize(&highResImg, (int)(expActuaVideoSize.x), (int)(expActuaVideoSize.y));
    //UnloadImage(highResImg);
    return LoadImageFromTexture(expTexture.texture);
}