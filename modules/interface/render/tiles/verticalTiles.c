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
#include <synth.h>
#include <export.h>
#include "tiles.h"
#include <tinyfiledialogs.h>
#include <threads.h>
#include <waterfall.h>
#include <handler.h>


#define NUM_OF_INT_SETTINGS 6
#define NUM_OF_FLOAT_SETTINGS 5
#define NUM_OF_SETTINGS 11

#define PROJECT_TITLE_PLACEHOLDER "Project Title"
#define TRACK_TITLE_PLACEHOLDER "Track Title"



static Button backButton=NULL, previousButton=NULL, pausePlayButton=NULL, nextButton=NULL, videoButton=NULL;
static Color tilesBackgroundColor={0,0,0,0}, disabledBackgroundColor={0,0,0,0}, textOrHoverBackgroundColor={0,0,0,0}, settingsBackgroundColor={0,0,0,0};
Rectangle previewDiv={0,0,0,0}, previewRect={0,0,0,0};
static Rectangle settingsDiv={0,0,0,0}, controlDiv={0,0,0,0}, timerRect={0,0,0,0}, titleRect={0,0,0,0};
struct tiles_settings settings;
static float settingsSeperatorX=0, settingsScroll=0.0, settingsScrollTarget=0.0, settingsTotalHeight=0.0, settingsDivTargetHeight=0.0, settingsReferenceSize=10.0;
static Button settingsDivBtns[NUM_OF_SETTINGS]={NULL}, settingsLeftRightBtns[NUM_OF_INT_SETTINGS][2]={{NULL}};
static Slider settingsFloatSliders[NUM_OF_FLOAT_SETTINGS]={NULL}, timeSlider=NULL, settingsSlider=NULL;
static int beatsInMeasure=4, isPlaying=0, ffmpegAvailable=0, expVideoActive=0;
static double previewAspectRatio=9.0/16.0, curTime=0;
double beatDuration=1.0;
struct view_data view;
float divsPixelRadius=0;


static struct duration _generateDuration(enum setting_bool timeNotation, int integer, double floatingPoint) {
    struct duration ret;
    if (timeNotation) {
        ret.beats.beats = integer;
        ret.beats.remainder = (float)fmod(floatingPoint, 1.0);
        ret.seconds = (integer+ret.beats.remainder)*beatDuration;

        if ((uint32_t)(ret.beats.remainder+1e-4)>999) {
            ret.beats.beats += 1;
            ret.beats.remainder=0.0;
        }
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
        view.columns[i-settings.info.minShownKey] = (struct note_column){.key=i, .isDown=0, .holdForce=0, .color=BLACK, .x=x, .w=offsets[mod]};
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

static void _updateTracksAndNotesInInfo() {
    if (!settings.tracks) return;
    int tracks=0;
    uint32_t notes=0;
    int num=settings.totalTracks;

    for (int i=0; i<num; i++) {
        if (settings.tracks[i].show) {
            tracks++;
            notes += settings.tracks[i].totalNotes;
        }
    }

    settings.info.tracks = tracks;
    settings.info.notes = notes;
}

static void updateMinMaxKey() {
    int minKey=127, maxKey=0;
    uint64_t num = settings.notes.size;
    Note* arr = settings.notes.notes;

    for (uint64_t i=0; i<num; i++) {
        Note nt = arr[i];
        if (!(settings.tracks[nt->track].show)) continue;
        int key = nt->key;
        if (key<minKey) minKey=key;
        if (key>maxKey) maxKey=key;
    }


    settings.info.minKey = minKey;
    settings.info.maxKey = maxKey;
    settings.info.keys = 1+maxKey-minKey;

    if (minKey>maxKey) {
        minKey=60, maxKey=64;
    }
    while (maxKey-minKey<18) {
        if (minKey>0) minKey--;
        if (maxKey<127) maxKey++;
    }

    while (1) {
        uint8_t t=minKey%12;
        if (minKey<0) minKey=0, t=0;
        if (t==5 || t==0) break;
        minKey--;
    }
    while (1) {
        uint8_t t=maxKey%12;
        if (maxKey>127) maxKey=131, t=11;
        if (t==11 || t==4) break;
        maxKey++;
    }


    settings.info.minShownKey = minKey;
    settings.info.maxShownKey = maxKey;
    settings.info.shownKeys = 1+maxKey-minKey;

    printf("`updateMinMaxKey`: minKey=%u, maxKey=%u, minShownKey=%u, maxShownKey=%u, shownKeys=%u\n", settings.info.minKey, settings.info.maxKey, settings.info.minShownKey, settings.info.maxShownKey, settings.info.shownKeys);
}

static void _updateInfoChunk(int drumsChanged) {
    if (drumsChanged) {
        //settings.info = projectGetTilesInfo(settings.showDrums);
        settings.info.duration = projectGetDuration().time;
        _updateTracksAndNotesInInfo();
        updateMinMaxKey();
    }
    settings.totalDuration = _generateDuration(0, 0, settings.startDelay.seconds+settings.endDelay.seconds+settings.info.duration);
    
    if (drumsChanged) {
        uint8_t octave[] = {0,1,0,1,0,0,1,0,1,0,1,0};
        int whiteKeys=0;
        for (uint8_t i=settings.info.minShownKey; i<=settings.info.maxShownKey; i++) {
            whiteKeys += !(octave[i%12]);
        }

        view.whiteKeys = whiteKeys;
    }

    //if (drumsChanged) {
    //    if (settings.notes.notes) projectFreeNoteArray(settings.notes);
    //    settings.notes = projectGetNoteArray(settings.showDrums);
    //}

    _updateView();
    printf("`_updateInfoChunk`: view.columns=%p\n", (void*)view.columns);
}

static void _updateAspectRatio();


static void _freeTracksSettings() {
    if (!(settings.tracks)) return;

    struct track_tile_settings* tracks = settings.tracks;
    int tracksNum=projectGetTracksNum();
    for (int i=0; i<tracksNum; i++) {
        if (tracks[i].base) buttonFree(tracks[i].base);
        tracks[i].base = NULL;

        if (tracks[i].colorBtn) buttonFree(tracks[i].colorBtn);
        tracks[i].colorBtn = NULL;

        if (tracks[i].showBtn) buttonFree(tracks[i].showBtn);
        tracks[i].showBtn = NULL;

        for (int j=0; j<3; j++) {
            if (tracks[i].sld[j]) sliderFree(tracks[i].sld[j]);
            tracks[i].sld[j] = NULL;
        }
    }

    free(settings.tracks);
    settings.tracks = NULL;
}

static void _generateTracksForSettings() {
    int tracksNum=projectGetTracksNum();
    settings.totalTracks = tracksNum;
    struct track_tile_settings* tracks = malloc(sizeof(struct track_tile_settings)*tracksNum);

    if (!tracks) return;
    if (settings.tracks) _freeTracksSettings();
    settings.tracks = tracks;

    Rectangle rect={0,0,20,20};

    for (int i=0; i<tracksNum; i++) {
        Track tr = trackGetAtIdx(i);
        tracks[i].track = tr;
        tracks[i].color = getTrackThemeColor(i);
        tracks[i].show = (trackGetProgram(tr)<128)||(settings.showDrums==SETTING_ON);
        tracks[i].totalNotes = trackGetNumOfNotes(tr);
        tracks[i].colorMode = 0;    // load from track
        tracks[i].base = buttonCreate(rect, 0.25);
        buttonUpdateCursorOnHover(tracks[i].base, MOUSE_CURSOR_ARROW);
        tracks[i].colorBtn = buttonCreate(rect, 0.25);
        tracks[i].showBtn = buttonCreate(rect, 0.25);

        tracks[i].sld[0] = sliderCreate(rect, 1.0);
        tracks[i].sld[1] = sliderCreate(rect, 1.0);
        tracks[i].sld[2] = sliderCreate(rect, 1.0);
        sliderUpdateCursorOnHover(tracks[i].sld[0], MOUSE_CURSOR_RESIZE_EW);
        sliderUpdateCursorOnHover(tracks[i].sld[1], MOUSE_CURSOR_RESIZE_EW);
        sliderUpdateCursorOnHover(tracks[i].sld[2], MOUSE_CURSOR_RESIZE_EW);

    }
}



void verticalTilesInit() {
    printf("`verticalTilesInit`: entered\n");
    Rectangle rect = {0,0,20,20};

    ffmpegAvailable = isFFmpegAvailable();

    if (backButton) buttonFree(backButton);
    backButton = buttonCreate(rect, 0.35);

    if (previousButton) buttonFree(previousButton);
    previousButton = buttonCreate(rect, 0.35);
    if (pausePlayButton) buttonFree(pausePlayButton);
    pausePlayButton = buttonCreate(rect, 0.35);
    if (nextButton) buttonFree(nextButton);
    nextButton = buttonCreate(rect, 0.35);
    if (videoButton) buttonFree(videoButton);
    videoButton = ffmpegAvailable?buttonCreate(rect, 0.35):NULL;


    beatsInMeasure = globalHandlerGetBeatsInMeasure();
    beatDuration = globalHandlerGetBeatDuration();

    printf("`verticalTilesInit`: right before settings init\n");


    settings = (struct tiles_settings) {
        .beatsInMeasure = beatsInMeasure,

        .aspectRatio = ASPECT_RATIO_9_16,
        .theme = THEME_PROJECT,
        .timeNotation = SETTING_ON,     // 0 -> seconds, 1 -> beats 
        .countMeasures = SETTING_ON,
        .identifyChords = SETTING_OFF,
        .showDrums = SETTING_OFF,
        
        .delayConstraints = (struct float_constraints){.min=0, .max=beatDuration*beatsInMeasure*4},
        .startDelay = _generateDuration(1, beatsInMeasure, 0.0),
        .endDelay = _generateDuration(1, beatsInMeasure, 0.0),

        .visDurConstraints = (struct float_constraints){.min=beatDuration*2, .max=beatDuration*beatsInMeasure*10},
        .visibleDuration = _generateDuration(1, beatsInMeasure*3, 0.0),

        .keyHeightConstraints = (struct float_constraints){.min=0.05, .max=0.3},
        .keyHeight = 0.1,

        .spacingConstraints = (struct float_constraints){.min=0, .max=0.1},
        .space = 0.0,

        .totalTracks = 0,
        .tracks = NULL,

        .notes = projectGetNoteArray(1),

    };

    view = (struct view_data){
        .totalKeys=0,
        .whiteKeys=0,
        .spacePerc = 0.02,
        .whiteKeyPerc = 0.1,
        .keyHeight = 0.1,
        .blackKeyWidth=0.6,
        .blackKeyHeight=0.6,

        .columns=NULL,
    };

    printf("`verticalTilesInit`: right after settings init\n");


    for (int i=0; i<NUM_OF_INT_SETTINGS; i++) {
        if (settingsLeftRightBtns[i][0]) buttonFree(settingsLeftRightBtns[i][0]);
        if (settingsLeftRightBtns[i][1]) buttonFree(settingsLeftRightBtns[i][1]);
        settingsLeftRightBtns[i][0] = buttonCreate(rect, 0.25);
        settingsLeftRightBtns[i][1] = buttonCreate(rect, 0.25);
    }

    for (int i=0; i<NUM_OF_SETTINGS; i++) {
        if (settingsDivBtns[i]) buttonFree(settingsDivBtns[i]);
        settingsDivBtns[i] = buttonCreate(rect, 0.25);
        buttonUpdateCursorOnHover(settingsDivBtns[i], MOUSE_CURSOR_ARROW);
    }

    for (int i=0; i<NUM_OF_FLOAT_SETTINGS; i++) {
        if (settingsFloatSliders[i]) sliderFree(settingsFloatSliders[i]);
        settingsFloatSliders[i] = sliderCreate(rect, 0.35);
        sliderUpdateCursorOnHover(settingsFloatSliders[i], MOUSE_CURSOR_RESIZE_EW);
    }

    if (timeSlider) sliderFree(timeSlider);
    timeSlider = sliderCreate(rect, 1.0);

    if (settingsSlider) sliderFree(settingsSlider);
    settingsSlider = sliderCreate(rect, 1.0);
    sliderUpdateCursorOnHover(settingsSlider, MOUSE_CURSOR_RESIZE_NS);

    isPlaying=0;
    curTime=0;
    expVideoActive=0;
    settingsScroll=0.0, settingsScrollTarget=0.0;

    _generateTracksForSettings();
    _updateInfoChunk(1);
    _updateAspectRatio();

    printf("`verticalTilesInit`: exited\n");
}

static void closeTileSettings() {
    if (settings.notes.notes) projectFreeNoteArray(settings.notes);
    settings.notes.size = 0;
    settings.notes.notes = NULL;

    _freeTracksSettings();

    if (view.columns) free(view.columns);
    view.columns = NULL;
}

void verticalTilesClose() {
    if (backButton) buttonFree(backButton);
    backButton = NULL;

    if (previousButton) buttonFree(previousButton);
    previousButton = NULL;
    if (pausePlayButton) buttonFree(pausePlayButton);
    pausePlayButton = NULL;
    if (nextButton) buttonFree(nextButton);
    nextButton = NULL;
    if (videoButton) buttonFree(videoButton);
    videoButton = NULL;

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

    if (settingsSlider) sliderFree(settingsSlider);
    settingsSlider = NULL;


    expVideoActive=0;

    closeTileSettings();
}


void verticalTilesVideoExportInit() {
    expVideoActive = 1;
    tilesExportVideoInit();
}

void verticalTilesVideoExportClose() {
    expVideoActive = 0;
    tilesExportVideoClose();
}


static const char* _getSettingStringForAspectRatio() {
    switch (settings.aspectRatio) {
        case ASPECT_RATIO_9_16: return "9:16";
        case ASPECT_RATIO_3_4: return "3:4";
        case ASPECT_RATIO_1_1: return "1:1";
        case ASPECT_RATIO_4_3: return "4:3";
        case ASPECT_RATIO_16_9: return "16:9";

        default: return "Unavailable";
    }
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

static void _updateAspectRatio() {
    double aspectRatio[ASPECT_RATIO_END] = {
        [ASPECT_RATIO_9_16] = 9.0/16.0,
        [ASPECT_RATIO_3_4] = 3.0/4.0,
        [ASPECT_RATIO_1_1] = 1.0,
        [ASPECT_RATIO_4_3] = 4.0/3.0,
        [ASPECT_RATIO_16_9] = 16.0/9.0,
    };
    previewAspectRatio = aspectRatio[settings.aspectRatio];
}

void tilesSettingsGetResolution(int* width, int* height) {
    int res[ASPECT_RATIO_END][2] = {
        [ASPECT_RATIO_9_16] = {1080, 1920},
        [ASPECT_RATIO_3_4] = {1080, 1440},
        [ASPECT_RATIO_1_1] = {1080, 1080},
        [ASPECT_RATIO_4_3] = {1440, 1080},
        [ASPECT_RATIO_16_9] = {1920, 1080},
    };
    if (width) *width = res[settings.aspectRatio][0];
    if (height) *height = res[settings.aspectRatio][1];
}

double tilesSettingsGetTotalDuration() {
    return settings.totalDuration.seconds;
}

void tilesSettingsGetDelays(double* startDelay, double* endDelay) {
    if (startDelay) *startDelay = settings.startDelay.seconds;
    if (endDelay) *endDelay = settings.endDelay.seconds;
}

double _tilesControlGetTime() {
    return curTime;
}

static void _settingArrowPressed(int idx, int leftRight) {
    if (idx<0 || leftRight<0 || leftRight>1) return;
    switch (idx) {
        case 0: {
            if (leftRight) settings.aspectRatio = (settings.aspectRatio+1)%ASPECT_RATIO_END;
            else settings.aspectRatio = settings.aspectRatio?(settings.aspectRatio-1):ASPECT_RATIO_END-1;
            _updateAspectRatio();
            return;
        }

        case 1: {
            if (leftRight) settings.theme = (settings.theme+1)%THEME_END;
            else settings.theme = settings.theme?(settings.theme-1):THEME_END-1;
            return;
        }

        case 2: {
            settings.timeNotation = !(settings.timeNotation);
            return;
        }

        case 3: {
            settings.countMeasures = !(settings.countMeasures);
            return;
        }

        case 4: {
            settings.identifyChords = !(settings.identifyChords);
            return;
        }

        case 5: {
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
    synthPanic();
    verticalTilesClose();
    globalHandlerSetRenderType(ART_REGULAR);
}

static void _btnPlayPauseClicked() {
    if (isPlaying) {
        isPlaying=0;
        synthPanic();
    }
    else {
        if (curTime>=settings.totalDuration.seconds) curTime=0;
        if (settings.totalDuration.seconds>0) isPlaying=1;
    }
}

static void _btnPreviousClicked() {
    if (curTime>settings.startDelay.seconds) {
        double f = curTime-settings.startDelay.seconds;
        double flr = (beatDuration*beatsInMeasure)*floor(f/(beatDuration*beatsInMeasure));
        if (f>flr+1e-4) curTime=flr+settings.startDelay.seconds;
        else curTime=flr-(beatDuration*beatsInMeasure)+settings.startDelay.seconds;
    } else curTime = 0;
}

static void _btnNextClicked() {
    if (curTime>settings.totalDuration.seconds-settings.endDelay.seconds) {
        curTime = settings.totalDuration.seconds;
    } else if (curTime>=settings.startDelay.seconds) {
        double f = curTime-settings.startDelay.seconds;
        double cl = (beatDuration*beatsInMeasure)*ceil(f/(beatDuration*beatsInMeasure));
        if (f<cl-1e-4) curTime=cl+settings.startDelay.seconds;
        else curTime=cl+(beatDuration*beatsInMeasure)+settings.startDelay.seconds;
    } else curTime = settings.startDelay.seconds;
}

static void _actionLaunchVideoSubprocess() {
    isPlaying = 0;
    synthPanic();

    const char* projectTitle = projectGetCurrentTitle();
    if (!projectTitle) projectTitle = PROJECT_TITLE_PLACEHOLDER;

    char* title = stringToFileName(projectTitle, 30);
    char* conct = concatenateStrings(title, ".mp4");
    free(title);

    const char* path = tinyfd_saveFileDialog("Export Waterfall", conct, 1, (const char *[]){"*.mp4"}, "MP4 Format");
    free(conct);

    if (path) {
        expVideoActive = 1;
        threadRequestExportVideoTilesWaterfall(path);
    }
}

static void _settingsScroll(float dz) {
    settingsScrollTarget += dz;
    //settingsScrollMax = (NUM_OF_SETTINGS+2.2*settings.totalTracks);
    float tmp = settingsTotalHeight/settingsReferenceSize;
    if (settingsScrollTarget<0) settingsScrollTarget=0.0;
    else if (settingsScrollTarget>tmp) settingsScrollTarget=tmp;
}

static void _settingsScrollSmoothly() {
    settingsScroll += 0.2*(settingsScrollTarget-settingsScroll);
}

static void _updateTilesButtons() {
    float disableHover = !CheckCollisionPointRec(globalMouseHandler.pos, settingsDiv);
    float rh=0.05*screenSize.y;

    settingsDivTargetHeight = settingsDiv.height;
    settingsTotalHeight = (rh+interfaceSpace1)*(NUM_OF_SETTINGS+4)+2*(rh+2*interfaceSpace2)+(rh*2.2+interfaceSpace1)*settings.totalTracks;
    settingsReferenceSize = rh;

    float dz = globalMouseHandler.scroll;
    _settingsScroll(-2.0*dz*(!disableHover));

    if (settingsSlider) {
        Rectangle rect = {settingsDiv.x+settingsDiv.width+(interfaceSpace2-interfaceSpace1)*0.5, settingsDiv.y, interfaceSpace1, settingsDiv.height-interfaceSpace2};
        sliderUpdateRectangle(settingsSlider, rect);
        sliderUpdateSlideValue(settingsSlider, settingsScroll*rh/(settingsTotalHeight-settingsDivTargetHeight));
        sliderUpdate(settingsSlider, -1);


        if (isSliderDragged(settingsSlider)) {
            float th = rect.height*settingsDivTargetHeight/settingsTotalHeight;
            float mposClip = floatClip(globalMouseHandler.pos.y, rect.y+0.5*th, rect.y+rect.height-0.5*th);
            float nval = (mposClip-rect.y-0.5*th)/(rect.height-th);
            settingsScrollTarget = nval*(settingsTotalHeight-settingsDivTargetHeight)/rh;
        }

    }

    _settingsScrollSmoothly();

    float l = controlLineHeight;
    float x = 0.2*l;
    if (backButton) {
        Rectangle rect = {x, 0.2*l, 0.8*l, 0.8*l};
        buttonUpdateRectangle(backButton, rect);
        buttonUpdate(backButton, -1);
        if (isButtonClicked(backButton)) actionDefer(_backToRegularRender);
    }

    x += 0.8*l+interfaceSpace2;

    Button btns[] = {previousButton, pausePlayButton, nextButton, videoButton};
    int allowed[] = {!isPlaying && curTime>0, settings.totalDuration.seconds>0, !isPlaying && curTime<settings.totalDuration.seconds, settings.totalDuration.seconds>0};
    OnClickFunc funcs[] = {_btnPreviousClicked, _btnPlayPauseClicked, _btnNextClicked, _actionLaunchVideoSubprocess};
    for (int i=0; i<4; i++) {
        Button btn = btns[i];
        if (btn) {
            Rectangle rect = {x, 0.2*l, 0.8*l, 0.8*l};
            buttonUpdateRectangle(btn, rect);
            if (allowed[i]) buttonEnable(btn);
            else buttonDisable(btn);
            buttonUpdate(btn, -1);
            if (funcs[i] && isButtonClicked(btn)) actionDefer(funcs[i]);
            x += 0.8*l+interfaceSpace1;
        }
    }

    x += interfaceSpace2-interfaceSpace1;

    timerRect = (Rectangle){x,0.2*l,3*0.8*l,0.8*l};
    x += timerRect.width+interfaceSpace2;
    titleRect = (Rectangle){x,0.2*l,screenSize.x-0.5*interfaceSpace2-x,0.8*l};

    float offsetY = settingsScroll*rh;

    Rectangle fill = {interfaceSpace1,settingsDiv.y+interfaceSpace2+rh+interfaceSpace1-offsetY,settingsDiv.width-2*interfaceSpace1,rh};
    for (int i=0; i<NUM_OF_SETTINGS; i++) {
        Button btn = settingsDivBtns[i];
        if (btn) {
            buttonUpdateRectangle(btn, fill);
            if (disableHover) buttonDisableHover(btn);
            else buttonEnableHover(btn);
            buttonUpdate(btn, -1);
        }
        fill.y += rh+interfaceSpace1;
    }

    float y=settingsDiv.y+interfaceSpace2+rh+interfaceSpace1-offsetY, w=rh-2*interfaceSpace1;
    for (int i=0; i<NUM_OF_INT_SETTINGS; i++) {
        Button btn1=settingsLeftRightBtns[i][0], btn2=settingsLeftRightBtns[i][1];

        if (btn1) {
            Rectangle rect = {settingsSeperatorX+interfaceSpace1, y+interfaceSpace1, w, w};
            buttonUpdateRectangle(btn1, rect);
            if (disableHover) buttonDisableHover(btn1);
            else buttonEnableHover(btn1);
            buttonUpdate(btn1, -1);
            if (isButtonClicked(btn1)) actionDefer(someSettingButtonPressed);
        }

        if (btn2) {
            Rectangle rect = {settingsDiv.x+settingsDiv.width-interfaceSpace1-w, y+interfaceSpace1, w, w};
            buttonUpdateRectangle(btn2, rect);
            if (disableHover) buttonDisableHover(btn2);
            else buttonEnableHover(btn2);
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
            if (disableHover) sliderDisableHover(sld);
            else sliderEnableHover(sld);
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


    if (settings.tracks) {
        for (int i=0; i<settings.totalTracks; i++) {
            struct track_tile_settings trackSet = settings.tracks[i];
            int isDrums = (trackGetProgram(trackSet.track)>127);
            int forceHide = (isDrums && !settings.showDrums);
            int hidden=forceHide;

            Rectangle rect={interfaceSpace1,y,settingsDiv.width-2*interfaceSpace1,2.2*rh};
            buttonUpdateRectangle(trackSet.base, rect);
            if (disableHover) buttonDisableHover(trackSet.base);
            else buttonEnableHover(trackSet.base);
            buttonUpdate(trackSet.base, -1);

            float btnWdh = 0.4*rect.height, vertSpace=(rect.height-2*btnWdh)/3.0;
            Rectangle trect = {rect.x+rect.width-btnWdh-interfaceSpace2,rect.y+rect.height-vertSpace-btnWdh, btnWdh, btnWdh};

            buttonUpdateRectangle(trackSet.showBtn, trect);
            if (forceHide) buttonDisable(trackSet.showBtn);
            else buttonEnable(trackSet.showBtn);
            if (disableHover) buttonDisableHover(trackSet.showBtn);
            else buttonEnableHover(trackSet.showBtn);
            buttonUpdate(trackSet.showBtn, -1);
            if (isButtonClicked(trackSet.showBtn)) {
                settings.tracks[i].show = 1-trackSet.show;
                _updateInfoChunk(1);
            }
            hidden = (hidden || !(settings.tracks[i].show));

            trect.y -= trect.height+vertSpace;
            buttonUpdateRectangle(trackSet.colorBtn, trect);
            if (hidden) buttonDisable(trackSet.colorBtn);
            else buttonEnable(trackSet.colorBtn);
            if (disableHover) buttonDisableHover(trackSet.colorBtn);
            else buttonEnableHover(trackSet.colorBtn);
            buttonUpdate(trackSet.colorBtn, -1);
            if (isButtonClicked(trackSet.colorBtn)) settings.tracks[i].color = getTrackThemeColor(i);
            
            trackSet = settings.tracks[i];
            float btnHght=0.1*rect.height;
            btnWdh=0.25*rect.width, vertSpace=(rect.height-3*btnHght)/4.0;
            trect = (Rectangle){trect.x-btnWdh-interfaceSpace2,rect.y+vertSpace, btnWdh, btnHght};

            float vals[] = {trackSet.color.r/255.0, trackSet.color.g/255.0, trackSet.color.b/255.0};

            for (int j=0; j<3; j++) {
                Slider sld = trackSet.sld[j];
                sliderUpdateRectangle(sld, trect);
                if (hidden) sliderDisable(sld);
                else sliderEnable(sld);
                if (disableHover) sliderDisableHover(sld);
                else sliderEnableHover(sld);
                sliderUpdate(sld, -1);

                if (isSliderDragged(sld)) vals[j] = sliderUpdateValueCommonHorizontal(sld);
                else sliderUpdateSlideValue(sld, vals[j]);

                trect.y += btnHght+vertSpace;
            }

            settings.tracks[i].color = (Color){(unsigned char)(255*vals[0]), (unsigned char)(255*vals[1]), (unsigned char)(255*vals[2]), 255};
            y += rect.height+interfaceSpace1;
        }
    }


    if (timeSlider) {
        float h = floatMax(6,0.009*screenSize.y);
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
    settingsDiv = (Rectangle){0, controlDiv.y+controlDiv.height+interfaceSpace2, 0.32*screenSize.x, screenSize.y-(controlDiv.y+controlDiv.height+interfaceSpace2)};
    settingsSeperatorX = settingsDiv.x+0.5*settingsDiv.width;
    previewDiv = (Rectangle){settingsDiv.x+settingsDiv.width+2*interfaceSpace2, settingsDiv.y, screenSize.x-(settingsDiv.x+settingsDiv.width+interfaceSpace2), settingsDiv.height};

    Rectangle tempRect = {-0.5*settingsDiv.width,settingsDiv.y,settingsDiv.width*1.5,settingsDiv.height*1.5};
    divsPixelRadius = getRadiusForRoundedRectangle(tempRect, 0.08);

    previewRect = rectangleScaleToFitInCenter((Vector2){(float)previewAspectRatio,1.0}, previewDiv);
    if (previewRect.width>previewDiv.width-2*divsPixelRadius) previewRect = rectangleScaleToFitInCenter((Vector2){(float)previewAspectRatio,1.0}, (Rectangle){previewDiv.x+divsPixelRadius,previewDiv.y,previewDiv.width-divsPixelRadius,previewDiv.height});


    tilesSetUpTargetRectangle(previewRect);
    tilesSetUpColors(tilesBackgroundColor, disabledBackgroundColor, textOrHoverBackgroundColor, settingsBackgroundColor);

    if (IsKeyPressed(KEY_SPACE)) {
        if (isPlaying) {
            isPlaying=0;
            synthPanic();
        }
        else {
            if (curTime>=settings.totalDuration.seconds) curTime=0;
            if (settings.totalDuration.seconds>0) isPlaying=1;
        }
    }
    double oldT=curTime;
    double dt = GetFrameTime();
    if (isPlaying) curTime+=dt;
    if (curTime>=settings.totalDuration.seconds) {
        curTime=settings.totalDuration.seconds;
        isPlaying=0;
        synthPanic();
    }
    if (curTime<0) curTime=0.0;
    if (oldT<curTime) globalHandlerPlayEventsInRange(oldT-settings.startDelay.seconds, curTime-settings.startDelay.seconds);
}

void order2precomputeVerticalTiles() {
    if (expVideoActive) return;

    _updateTilesValues();
    _updateTilesButtons();
}

static float _getTextSizeToFitInRect(Rectangle rect, float defaultSize, const char* text, float spacing) {
    if (!text) return defaultSize;

    float sizeH = textFontGetSize(GlobalFonts[0].font, "0A", defaultSize, 0).y;
    float sizeX = textFontGetSize(GlobalFonts[0].font, text, defaultSize, 0).x;
    float finalSize = defaultSize;
    if (sizeX>rect.width-spacing) finalSize=floatMin(finalSize, defaultSize*(rect.width-spacing)/sizeX);
    if (sizeH>rect.height-spacing) finalSize=floatMin(finalSize, defaultSize*(rect.height-spacing)/sizeH);
    return finalSize;
}

static void _renderVerticalSeperator(float x, float ty, float h, float screenX) {
    Color col = COLOR_TEXT_4; col.a=50;
    DrawLineEx((Vector2){x, ty+0.2*h}, (Vector2){x, ty+0.8*h}, floatMax(1, screenX*0.002), col);
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

    _renderVerticalSeperator(rect.x+rect.width+0.5*interfaceSpace2, controlDiv.y, controlDiv.height, screenSize.x);
}

static void _renderTimeSlider() {
    if (!timeSlider) return;

    Rectangle rect = sliderGetRectangle(timeSlider);
    Rectangle trect = sliderGetRectangleValueCommon(timeSlider);

    DrawRectangleRec(rect, settingsBackgroundColor);
    DrawRectangleRec(trect, COLOR_TRACK_THEME_5);

    Color top=COLOR_BACKGROUND_1, bottom=COLOR_BACKGROUND_1;
    top.a=200, bottom.a=0;
    DrawRectangleGradientEx((Rectangle){rect.x,rect.y+rect.height,rect.width,1.5*rect.height}, top, bottom, bottom, top);
}

static void _renderControlButtons() {
    Button btns[] = {previousButton, pausePlayButton, nextButton, videoButton};

    enum icon_title btnIcons[] = {T_ICON_PREVIOUS, isPlaying?T_ICON_PAUSE:T_ICON_PLAY, T_ICON_NEXT, T_ICON_VIDEO};
    float sizes[] = {0.85, isPlaying?0.8:0.65, 0.85, 0.92}, endx=0;
    Color col1=textOrHoverBackgroundColor, col2={32,34,40,255}, col3={45,32,34,255};
    Color cols[]={col2, col2, col2, col3}, colsFg[]={COLOR_PALETTE_1_P9, COLOR_PALETTE_1_P9, COLOR_PALETTE_1_P9, (Color){255, 180, 192, 255}};
    int s = sizeof(btns)/sizeof(Button);
    

    for (int i=0; i<s; i++) {
        Button btn = btns[i];
        if (btn) {
            Rectangle rect = buttonGetRectangle(btn);
            endx = rect.x+rect.width;
            float effect = buttonGetEffectValue(btn);
            Rectangle trect = scaleRctangleFromCenter(rect, lerp(0.3, 1, effect));
            int enabled = isButtonEnabled(btn);

            Color tmp1=col1, tmp2=cols[i];
            if (!enabled) tmp1=disabledBackgroundColor;

            Color blendedCol = blendColors(tmp1, tmp2, effect);
            blendedCol.a = (unsigned char)(255*pow(effect, 0.3));
    
            DrawRectangleRounded(trect, getRoundnessForRoundedRectangleTransformation(rect, trect, buttonGetRoundness(btn), 0), 4, blendedCol);
            iconRerder(btnIcons[i], scaleRctangleFromCenter(rect, 0.95*sizes[i]*lerp(0.9, 0.95, effect)), blendColors(enabled?COLOR_TEXT_1:COLOR_TEXT_4, colsFg[i], effect));
        }
    }

    _renderVerticalSeperator(endx+0.5*interfaceSpace2, controlDiv.y, controlDiv.height, screenSize.x);

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
    if (settings.totalDuration.seconds<=0) return strdup("-:-- / -:--");
    double curT = floatMin(curTime, settings.totalDuration.seconds);
    return _generateTimestampString(curT, settings.totalDuration.seconds);
}


static void _renderControlTimerAndTitle() {
    DrawRectangleRounded(timerRect, 0.35, 4, (settings.totalDuration.seconds>0)?textOrHoverBackgroundColor:disabledBackgroundColor);
    char* dur = generateTimestampString();
    const char* tmp = (dur)?dur:"-:-- / -:--";
    float textSize = 0.035*floatMin(screenSize.x, screenSize.y);
    float finalSize = _getTextSizeToFitInRect(timerRect, textSize, tmp, interfaceSpace2);
    renderFontStringAlign(GlobalFonts[0].font, tmp, getRectangleCenter(timerRect), (Vector2){0.5, 0.5}, finalSize, 0, COLOR_PALETTE_1_P9);
    if (dur) {
        free(dur);
        dur=NULL;
    }

    _renderVerticalSeperator(titleRect.x-0.5*interfaceSpace2, controlDiv.y, controlDiv.height, screenSize.x);
    const char* title = projectGetCurrentTitle();
    if (!title || strlen(title)==0) title = PROJECT_TITLE_PLACEHOLDER;

    DrawRectangleRounded(titleRect, 0.35, 4, textOrHoverBackgroundColor);
    finalSize = _getTextSizeToFitInRect(titleRect, textSize, title, interfaceSpace2);
    renderFontStringAlign(GlobalFonts[0].font, title, getRectangleCenter(titleRect), (Vector2){0.5, 0.5}, finalSize, 0, COLOR_PALETTE_1_P9);
}

static void _renderControlDiv() {
    DrawRectangleRec(controlDiv, tilesBackgroundColor);
    _renderBackButton();
    _renderControlButtons();
    _renderControlTimerAndTitle();
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
    if (bkgRect.y>screenSize.y+1 || bkgRect.y+bkgRect.height<controlDiv.y+controlDiv.height-1) return;

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

static float _getSettingOptionsPixelRadius(Rectangle fill) {
    Rectangle bkgRect = {fill.x+interfaceSpace1, fill.y, fill.width-2*interfaceSpace1, fill.height};
    return getRadiusForRoundedRectangle(bkgRect, 0.35);
}

static void _renderInfoKeyValuePair(const char* key, const char* value, float textSize, Rectangle fill) {
    Rectangle bkgRect = {fill.x+interfaceSpace1, fill.y, fill.width-2*interfaceSpace1, fill.height};
    if (bkgRect.y>screenSize.y+1 || bkgRect.y+bkgRect.height<controlDiv.y+controlDiv.height-1) return;

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
    float finalSize = _getTextSizeToFitInRect(bkgRect, textSize, title, interfaceSpace1*2.0);
    renderFontStringAlign(GlobalFonts[0].font, title, (Vector2){x,y}, (Vector2){0.5, 0.5}, finalSize, 0, COLOR_TEXT_1);
}

static void _renderTrackSetting(int idx, float textSize, float pixelRadius) {
    if (idx<0 || idx>=settings.totalTracks || !settings.tracks) return;

    struct track_tile_settings trackSet = settings.tracks[idx];

    Rectangle bkgRect = buttonGetRectangle(trackSet.base);
    if (bkgRect.y>screenSize.y+1 || bkgRect.y+bkgRect.height<controlDiv.y+controlDiv.height-1) return;

    float divEffect = buttonGetEffectValue(trackSet.base);
    int shown = trackSet.show;
    int forceHidden = !isButtonEnabled(trackSet.showBtn);
    Color bkgCol = shown?blendColors(settingsBackgroundColor, WHITE, 0.02*divEffect):blendColors(settingsBackgroundColor, BLACK, 0.05+0.22*forceHidden);
    DrawRectangleRounded(bkgRect, getRoundnessForRoundedRectangle(bkgRect, pixelRadius), 4, bkgCol);
    
    DrawRectangleRounded((Rectangle){bkgRect.x+bkgRect.width-2*pixelRadius, bkgRect.y, pixelRadius*2, bkgRect.height}, 1.0, 4, blendColors(trackSet.color, bkgCol, 0.6*(!shown)+0.3*forceHidden));

    Rectangle tempRect = sliderGetRectangle(trackSet.sld[0]);

    float stx = bkgRect.x+0.55*bkgRect.height;
    Rectangle titleRect = {stx,bkgRect.y,tempRect.x-stx-interfaceSpace1, 0.7*bkgRect.height};
    const char* title = trackGetTitle(trackSet.track);
    if (!title || !strlen(title)) title = TRACK_TITLE_PLACEHOLDER;
    float spacing = 1.5*interfaceSpace1;
    float finalSize = _getTextSizeToFitInRect(titleRect, 0.85*textSize, title, spacing);
    renderFontStringAlign(GlobalFonts[0].font, title, (Vector2){titleRect.x+0.75*spacing, titleRect.y+0.5*titleRect.height}, (Vector2){0, 0.5}, finalSize, 0, blendColors(COLOR_TEXT_1, BLACK, 0.35*(!shown)+0.35*forceHidden));
    Color tmpCol = blendColors(trackSet.color, COLOR_TEXT_3, 0.75);
    tmpCol = blendColors(tmpCol, BLACK, 0.35*(!shown)+0.35*forceHidden);
    const char* programName = midiGetProgramName(trackGetProgram(trackSet.track));
    renderFontStringAlign(GlobalFonts[1].font, programName, (Vector2){titleRect.x+0.75*spacing, titleRect.y+bkgRect.height-0.5*titleRect.height}, (Vector2){0, 0.5}, fontGetTextSizeToFitInRect(GlobalFonts[1].font, titleRect, 0.85*finalSize, programName, spacing, spacing), 0, tmpCol);

    float perc = 0.4;
    Rectangle iconRect = {bkgRect.x+0.5*interfaceSpace2, bkgRect.y+(0.5-0.5*perc)*bkgRect.height,perc*bkgRect.height, perc*bkgRect.height};
    enum icon_title icTitle = midiGetProgramTypeIcon(trackGetProgram(trackSet.track));
    iconRerder(icTitle, scaleRctangleFromCenter(iconRect, normalizeProgramTypeIcon(icTitle)), blendColors(COLOR_TEXT_3, BLACK, 0.35*(!shown)+0.35*forceHidden));

    
    Button btns[] = {trackSet.colorBtn, trackSet.showBtn};
    enum icon_title icons[] = {T_ICON_REFRESH, trackSet.show?T_ICON_SHOWN:T_ICON_HIDDEN};
    float icScales[] = {0.5, 1.0};
    for (int i=0; i<2; i++) {
        Button btn = btns[i];
        Rectangle rct = buttonGetRectangle(btn);
        float effect = buttonGetEffectValue(btn);
        int isEnabled = isButtonEnabled(btn);

        DrawRectangleRounded(rct, getRoundnessForRoundedRectangle(rct, pixelRadius), 4, isEnabled?blendColors(bkgCol, WHITE, 0.02+0.02*effect):blendColors(bkgCol, BLACK, 0.05));
        iconRerder(icons[i], scaleRctangleFromCenter(rct, icScales[i]*(0.9+0.1*effect)), isEnabled?blendColors(COLOR_TEXT_3, blendColors(trackSet.color, COLOR_TEXT_1, 0.5), 0.25+0.75*effect):COLOR_TEXT_5);
    }

    for (int i=0; i<3; i++) {
        Slider sld = trackSet.sld[i];
        Rectangle rct = sliderGetRectangle(sld);
        float effect = sliderGetEffectValue(sld);
        float val = sliderGetSlideValue(sld);
        int isEnabled = !isSliderDisabled(sld);

        DrawRectangleRounded(rct, 1.0, 4, blendColors(bkgCol, WHITE, 0.02+0.02*effect));
        Color col;
        if (i==0) col = (Color){(unsigned char)(255.0*val), bkgCol.g, bkgCol.b, 255};
        else if (i==1) col = (Color){ bkgCol.r, (unsigned char)(255.0*val), bkgCol.b, 255};
        else if (i==2) col = (Color){bkgCol.r, bkgCol.g, (unsigned char)(255.0*val), 255};

        DrawRectangleRounded((Rectangle){rct.x+rct.height*0.15, rct.y+rct.height*0.15, rct.width*(1.0-0.3*rct.height/rct.width)*val, rct.height*0.7}, 1.0, 4, blendColors(bkgCol, col, (0.1+0.1*isEnabled+0.13*effect*isEnabled)+(0.5-0.13*effect*isEnabled+0.3*isEnabled)*val));


    }
}

static void _renderSettingsSlider() {
    if (!settingsSlider || isSliderDisabled(settingsSlider)) return;

    Rectangle rect = sliderGetRectangle(settingsSlider);
    float roundness = sliderGetRoundness(settingsSlider);
    float effect = sliderGetEffectValue(settingsSlider);
    float val = sliderGetSlideValue(settingsSlider);
    Color col = blendColors((Color){100,100,100,120}, (Color){180,180,180,160}, effect);

    float th = rect.height*settingsDivTargetHeight/settingsTotalHeight;
    Rectangle newRect = {rect.x, rect.y+(rect.height-th)*val, rect.width, th};
    DrawRectangleRounded(newRect, roundness, 4, col);

}

static void _renderSettings() {
    float textSize = 0.032*floatMin(screenSize.x, screenSize.y);
    float rh=0.05*screenSize.y;
    float offsetY = rh*settingsScroll;

    Rectangle tempRect = {-0.5*settingsDiv.width,settingsDiv.y-offsetY,settingsDiv.width*1.5,settingsDiv.height*1.5+offsetY};
    DrawRectangleRounded(tempRect, getRoundnessForRoundedRectangle(tempRect, divsPixelRadius), 8, tilesBackgroundColor);

    
    Rectangle rect = {0,settingsDiv.y+interfaceSpace2-offsetY,settingsDiv.width,rh};
    float pixelRadius = _getSettingOptionsPixelRadius(rect);
    _renderSettingsTitle("Settings", 1*textSize, rect);
    rect.y += rh+interfaceSpace1;

    const char* keys[] = {"Aspect Ratio", "Theme", "Time Notation", "Count Measures", "Identify Chords", "Show Drums", "Start Delay", "End Delay", "Visible Duration", "Key Height", "Spacing"};
    const char* vals[] = {_getSettingStringForAspectRatio(), _getSettingStringForTheme(), settings.timeNotation?"Beats":"Seconds", _getSettingStringForBool(settings.countMeasures), _getSettingStringForBool(settings.identifyChords), _getSettingStringForBool(settings.showDrums), _getSettingsStringForDuration(settings.startDelay), _getSettingsStringForDuration(settings.endDelay), _getSettingsStringForDuration(settings.visibleDuration), _getSettingStringForFloat(settings.keyHeight), _getSettingStringForFloat(settings.space)};
    enum setting_type types[] = {SETTING_INT, SETTING_INT, SETTING_BOOL, SETTING_BOOL, SETTING_BOOL, SETTING_BOOL, SETTING_FLOAT, SETTING_FLOAT, SETTING_FLOAT,SETTING_FLOAT,SETTING_FLOAT};
    int numOfOptions[] = {ASPECT_RATIO_END,THEME_END,2,2,2,2, 0,0,0,0,0};
    int selectedOptions[] = {settings.aspectRatio, settings.theme, settings.timeNotation, settings.countMeasures, settings.identifyChords, settings.showDrums, 0,0,0,0,0};
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



    if (settings.tracks) {
        for (int i=0; i<settings.totalTracks; i++) {
            _renderTrackSetting(i, textSize, pixelRadius);
        }
    }

    
    const char* nkeys[] = {"Total Duration", "Visible Keys", "Rendered Tracks", "Rendered Notes"};
    char* nvals[] = {_getSettingsStringForDuration(settings.totalDuration), _getSettingStringForUint32(settings.info.shownKeys), _getSettingStringForUint32(settings.info.tracks), _getSettingStringForUint32(settings.info.notes)};
    s = intMin(sizeof(nkeys)/sizeof(const char*), sizeof(nvals)/sizeof(char*));
    Rectangle tmp = (settings.tracks && settings.totalTracks>0)?buttonGetRectangle(settings.tracks[settings.totalTracks-1].base):buttonGetRectangle(settingsDivBtns[NUM_OF_SETTINGS-1]);
    rect.y = tmp.y+tmp.height+interfaceSpace2-interfaceSpace1;
    _renderSettingsTitle("Information", textSize, rect);
    rect.y += rh+interfaceSpace1;
    

    for (int i=0; i<s; i++) {
        _renderInfoKeyValuePair(nkeys[i], nvals[i], textSize, rect);
        if (nvals[i]) free(nvals[i]);
        nvals[i]=NULL;
        rect.y += rh+interfaceSpace1;
    }
    

    _renderSettingsSlider();
}



static void _renderDivSeperator() {
    float x = (settingsDiv.x+settingsDiv.width+previewDiv.x)*0.5;
    DrawLineEx((Vector2){x,controlDiv.y+controlDiv.height}, (Vector2){x,screenSize.y}, 2, COLOR_TEXT_4);
}



static void _hideImperfections() {
    DrawRectangleRec((Rectangle){previewDiv.x,0,screenSize.x-previewDiv.x,previewDiv.y}, COLOR_BACKGROUND_1);
}

void renderVerticalTiles() {
    if (expVideoActive) return;
    _renderSettings();
    _renderDivSeperator();

    Rectangle tmp = {previewDiv.x, previewDiv.y, previewDiv.width*1.5, previewDiv.height*1.5};
    DrawRectangleRounded(tmp, getRoundnessForRoundedRectangle(tmp, divsPixelRadius), 8, tilesBackgroundColor);

    _renderTilesToTargetRect();
    _hideImperfections();
    _renderControlDiv();
}