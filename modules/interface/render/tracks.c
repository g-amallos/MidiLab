#include <interface.h>
#include <backend.h>
#include <raylib.h>
#include <raymath.h>
#include <colors.h>
#include <utils.h>
#include <ui.h>
#include <stdlib.h>
#include <stdio.h>
#include <images.h>
#include <handler.h>
#include <math.h>
#include <synth.h>
#include <tinyfiledialogs.h>
#include <export.h>
#include <string.h>


#define TRACK_TITLE_PLACEHOLDER "Track Title"


float trackHeight=0, trackLeftWidth=0, trackCLineHeight=0, trackDivTargetHeight=0, trackScrollY=0, trackScrollYtarget=0, trackDivVisiblePosMin=0, trackDivVisiblePosMax=0;
int openLayout=0;
int timelineMeasureSkipsTop=1, timelineMeasureSkipsBottom=1, timelineBeatsSkipsTop=1, timelineBeatsSkipsBottom=1;
static int previouslySelectedTrack=-1, shouldUpdateAllTextures=0, maxTextureSize=0;


Rectangle trackDivLeftRect={0,0,0,0}, trackDivRightRect={0,0,0,0}, trackDivFullRect={0,0,0,0};
Color trackThemeColors[7], programTypeColors[MPT_END];
Button addTrackButton=NULL, timeLineDragButton=NULL;
Button bottomViewButtons[3]={NULL};
Slider trackSlider=NULL;


typedef struct track_control_ui {
    Track track;
    Button base;
    Button rightBase;
    Color theme;
    int themeIdx;
    float effect;
    uint8_t moving;
    float y;
    float shadow;
    enum icon_title icon;
    Textbox textbox;
    Slider slider;
    Button move;
    Button optionsButton;
    ButtonList btnList;

    RenderTexture2D preview;
} *TrackUI;

TrackUI tracks = NULL;



float normalizeProgramTypeIcon(enum icon_title iconType) {
    switch (iconType) {
        case T_ICON_PIANO: return 0.65;
        case T_ICON_PERCUSSION: return 1;
        case T_ICON_ORGAN: return 0.93;
        case T_ICON_GUITAR: return 1;
        case T_ICON_BASS: return 1;
        case T_ICON_VIOLIN: return 1;
        case T_ICON_CONTRABASS: return 1;
        case T_ICON_BRASS: return 0.85;
        case T_ICON_FLUTE: return 0.9;
        case T_ICON_KEYBOARD: return 1;
        case T_ICON_PAD: return 0.98;
        case T_ICON_EFFECTS: return 1;
        case T_ICON_MIDI: return 0.8;
        case T_ICON_DRUMS: return 1;
        default: return 1;
    }
}

static int getMaxTextureSize() {
    return 16384;
}


void initProgramTypeColors() {
    if (MPT_END<=0) return;
    float startingHue=200;
    float dh = 360.0/MPT_END;
    for (int i = 0; i<MPT_END; i++) {
        float hue = fmodf(dh*i+startingHue, 360.0);
        programTypeColors[i] = ColorFromHSV(hue, 0.58, 0.57);
    }
}


void renderTracksLeftInit() {
    trackThemeColors[0]=COLOR_TRACK_THEME_0;
    trackThemeColors[1]=COLOR_TRACK_THEME_1;
    trackThemeColors[2]=COLOR_TRACK_THEME_2;
    trackThemeColors[3]=COLOR_TRACK_THEME_3;
    trackThemeColors[4]=COLOR_TRACK_THEME_4;
    trackThemeColors[5]=COLOR_TRACK_THEME_5;
    trackThemeColors[6]=COLOR_TRACK_THEME_6;

    Rectangle rect = (Rectangle){20,20,20,20};
    addTrackButton = buttonCreate(rect, 0.25);
    timeLineDragButton = buttonCreate(rect, 0);
    trackSlider = sliderCreate(rect, 1);
    sliderUpdateCursorOnHover(trackSlider, MOUSE_CURSOR_RESIZE_NS);

    for (int i=0; i<3; i++) bottomViewButtons[i]=buttonCreate(rect, 0.25);

    initProgramTypeColors();
    maxTextureSize = getMaxTextureSize();
}

void freeTrackUIs() {
    if (tracks) {
        int totalTracks = projectGetTracksNum();
        for (int i=0; i<totalTracks; i++) {
            if (tracks[i].btnList) buttonListFree(tracks[i].btnList, 1);
            buttonFree(tracks[i].optionsButton);
            textboxFree(tracks[i].textbox);
            buttonFree(tracks[i].base);
            buttonFree(tracks[i].rightBase);
            sliderFree(tracks[i].slider);
            buttonFree(tracks[i].move);

            if (tracks[i].preview.id) UnloadRenderTexture(tracks[i].preview);
            tracks[i].preview.id = 0;
        }
        free(tracks);
        tracks=NULL;
    }
}


void customizeNewTrackUI(TrackUI tr, int idx) {
    if (!tr) return;
    Rectangle rect = (Rectangle){20, 20, 20, 20};
    tr->base = buttonCreate(rect, 0.2);
    tr->rightBase = buttonCreate(rect, 0.2);
    int cols = sizeof(trackThemeColors)/sizeof(Color);
    tr->themeIdx = GetRandomValue(0, cols-1);
    tr->theme = trackThemeColors[tr->themeIdx];
    tr->icon = T_ICON_PIANO;
    tr->textbox = textboxCreate(rect, 0.3, T_IN_STRING, (int)trackGetMaxTitleLength());
    tr->optionsButton = buttonCreate(rect, 0.5);
    tr->btnList = NULL;
    tr->slider = sliderCreate(rect, 1);
    sliderUpdateCursorOnHover(tr->slider, MOUSE_CURSOR_RESIZE_EW);
    tr->move = buttonCreate(rect, 0.2);
    tr->moving = 0;
    tr->y = idx+trackDivLeftRect.height/trackHeight+0.5;
    tr->effect = 0;
    tr->shadow = 0;

    tr->preview = (RenderTexture2D){0};
}

static inline int _trackPreviewPixelsPerBeat() {
    if (maxTextureSize>=16384) return 8;
    else if (maxTextureSize>=8192) return 4;
    else return 2;
}

void trackUIgenerateTrackPreview(int idx) {
    int totalTracks = projectGetTracksNum();
    if (idx<0 || idx>=totalTracks) return;

    if (tracks[idx].preview.id) UnloadRenderTexture(tracks[idx].preview);
    tracks[idx].preview.id = 0;
    Track track = trackGetAtIdx(idx);
    uint32_t notesNum = trackGetNumOfNotes(track);
    if (notesNum<=0) return;

    int keyMin = trackGetMinKey(track);
    int keyMax = trackGetMaxKey(track);
    if (keyMin<0 || keyMax<0 || keyMin>keyMax) return;

    double pixPerBeat = _trackPreviewPixelsPerBeat();
    uint32_t timestampEnd = trackGetTimestampEnd(track);
    uint32_t pcsInBts = trackPiecesInBeat();
    int width = intMax((int)(timestampEnd*pixPerBeat/pcsInBts), maxTextureSize);
    int height = keyMax-keyMin+1;

    
    int pad = 1+((height<20)?((20-height)/2):0);
    keyMin -= pad;
    keyMax += pad;
    height = keyMax-keyMin+1;
    

    TrackUI tr = tracks+idx;
    tr->preview = LoadRenderTexture(width, height);
    if (!(tr->preview.id)) return;

    SetTextureFilter(tr->preview.texture, TEXTURE_FILTER_POINT);

    Note* notes = trackGetNotes(track);
    BeginTextureMode(tr->preview);
    ClearBackground(BLANK);
    
    for (uint32_t i=0; i<notesNum; i++) {
        Note nt = notes[i];
        int x = (int)(nt->timestamp*pixPerBeat/pcsInBts);
        int w=(int)(((nt->timestamp+nt->duration)*pixPerBeat/pcsInBts)-x), y=nt->key-keyMin;
        DrawRectangle(x, y, w, 1, WHITE);
    }
    EndTextureMode();
}

void createTrackUIsFromScratch(uint8_t* colArr) {
    if (tracks) freeTrackUIs();

    int totalTracks = projectGetTracksNum();
    tracks = malloc(totalTracks*sizeof(struct track_control_ui));
    if (!tracks) return;

    for (int i=0; i<totalTracks; i++) {
        Track track = trackGetAtIdx(i);
        tracks[i].track = track;
        customizeNewTrackUI(tracks+i, i);
        tracks[i].themeIdx = ((int)colArr[i])%(sizeof(trackThemeColors)/sizeof(Color));
        tracks[i].theme = trackThemeColors[tracks[i].themeIdx];

        textboxLoadText(tracks[i].textbox, trackGetTitle(track));
        sliderUpdateSlideValue(tracks[i].slider, trackGetVelocity(track));

        //trackUIgenerateTrackPreview(i);
    }

    globalHandlerSelectTrack(1);
    synthPanic();
    shouldUpdateAllTextures = 1;
}

void renderTracksLeftClose() {
    if (addTrackButton) buttonFree(addTrackButton);
    addTrackButton = NULL;

    if (timeLineDragButton) buttonFree(timeLineDragButton);
    timeLineDragButton = NULL;

    if (trackSlider) sliderFree(trackSlider);
    trackSlider=NULL;

    for (int i=0; i<3; i++) {
        if (bottomViewButtons[i]) buttonFree(bottomViewButtons[i]);
        bottomViewButtons[i]=NULL;
    }

    freeTrackUIs();
}

int getTrackThemeColorIdx(int i) {
    int totalTracks = projectGetTracksNum();
    if (i<0 || i>=totalTracks) return -1;
    return tracks[i].themeIdx;
}

void setTrackThemeColorIdx(int tracki, int themei) {
    int totalTracks = projectGetTracksNum();
    if (tracki<0 || tracki>=totalTracks) return;
    tracks[tracki].themeIdx = themei%(sizeof(trackThemeColors)/sizeof(Color));
    tracks[tracki].theme = trackThemeColors[tracks[tracki].themeIdx];
}

void updateTrackUItitleAndPreview(int tracki) {
    int totalTracks = projectGetTracksNum();
    if (tracki<0 || tracki>=totalTracks) return;
    textboxLoadText(tracks[tracki].textbox, trackGetTitle(trackGetAtIdx(tracki)));
    trackUIgenerateTrackPreview(tracki);
}

Color getTrackThemeColor(int i) {
    int totalTracks = projectGetTracksNum();
    if (i<0 || i>=totalTracks) return (Color){0,0,0,0};
    return tracks[i].theme;
}

Color getSelectedTrackThemeColor() {
    int sel = globalHandlerGetSelectedTrack();
    if (sel<0) return (Color){0,0,0,0};
    int totalTracks = projectGetTracksNum();
    if (sel>=totalTracks) return (Color){0,0,0,0};
    return tracks[sel].theme;
}

Color getTrackThemeColorForWhiteKeys() {
    Color theme = getSelectedTrackThemeColor();
    return theme;   //blendColors(theme, (Color){200,200,200,255}, 0.05);
}

Color getTrackThemeColorForBlackKeys() {
    Color theme = getSelectedTrackThemeColor();
    return blendColors(theme, (Color){0,0,0,255}, 0.35);
}

Color getAnyTrackThemeColorForWhiteKeys(int i) {
    return getTrackThemeColor(i);
}

Color getAnyTrackThemeColorForBlackKeys(int i) {
    Color theme = getTrackThemeColor(i);
    return blendColors(theme, (Color){0,0,0,255}, 0.35);
}

static void scrollToShowSelectedTrack() {
    int totalTracks = projectGetTracksNum();
    float totalHeight = totalTracks*trackHeight+10;
    int idx = globalHandlerGetSelectedTrack();
    if (idx<0 || idx>=totalTracks) return;
    
    float y1=idx*trackHeight, y2=(idx+1)*trackHeight;
    if (y1>=trackScrollYtarget && y2<=trackScrollYtarget+trackDivTargetHeight) {
        trackScrollYtarget=trackScrollYtarget;
    } else if (y1<trackScrollYtarget) trackScrollYtarget=y1;
    else if (y2>trackScrollYtarget+trackDivTargetHeight-10) trackScrollYtarget=floatMin(y2-trackDivTargetHeight+10, y1);

    if (trackScrollYtarget>totalHeight-trackDivTargetHeight) trackScrollYtarget = totalHeight-trackDivTargetHeight; // totalHeight>trackDivTargetHeight && 
    if (trackScrollYtarget<0) trackScrollYtarget=0;
} 

void renderTrackCreateNew() {
    Track ntrack = trackCreateNew();
    int totalTracks = projectGetTracksNum();
    //globalHandlerSelectTrack(totalTracks-1);
    if (tracks) {
        TrackUI new = realloc(tracks, totalTracks*sizeof(struct track_control_ui));
        if (!new) return;   // We're cooked here
        tracks = new;
    } else {
        tracks = malloc(sizeof(struct track_control_ui));
        if (!tracks) return;
    }
    tracks[totalTracks-1].track = ntrack;
    customizeNewTrackUI(tracks+totalTracks-1, totalTracks-1);
    textboxLoadText(tracks[totalTracks-1].textbox, trackGetTitle(ntrack));
    sliderUpdateSlideValue(tracks[totalTracks-1].slider, trackGetVelocity(ntrack));
    
    scrollToShowSelectedTrack();
    synthPanic();
}

void renderTrackDeleteAtIdx(int idx) {
    int totalTracks = projectGetTracksNum();
    if (totalTracks<=idx || idx<0) return;

    trackDeleteAtIdx(idx);
    tracks[idx].track = NULL;
    buttonFree(tracks[idx].optionsButton);
    buttonListFree(tracks[idx].btnList, 1);
    textboxFree(tracks[idx].textbox);
    sliderFree(tracks[idx].slider);
    buttonFree(tracks[idx].base);
    buttonFree(tracks[idx].rightBase);

    if (tracks[idx].preview.id) UnloadRenderTexture(tracks[idx].preview);
    tracks[idx].preview.id = 0;

    for (int i=idx; i<totalTracks-1; i++) {
        tracks[i] = tracks[i+1];
    }
    
    if (totalTracks>1) tracks = realloc(tracks, (totalTracks-1)*sizeof(struct track_control_ui));
    else {
        free(tracks);
        tracks = NULL;
    }
}


void selectBottomViewNone() {
    globalHandlerSetKeyboardType(T_KEYBOARD_NONE);
}

void selectBottomViewHorizontal() {
    globalHandlerSetKeyboardType(T_KEYBOARD_HORIZONTAL);
}

void selectBottomViewVertical() {
    globalHandlerSetKeyboardType(T_KEYBOARD_VERTICAL);
}


void renderTrackCLineBackground() {
    DrawRectangleV((Vector2){0, controlLineHeight}, (Vector2){screenSize.x+2, trackCLineHeight}, COLOR_TRACK_C_LINE_BACKGROUND);
    //float ypos = controlLineHeight+trackCLineHeight;
    //DrawLineEx((Vector2){0, ypos}, (Vector2){screenSize.x+2, ypos}, 2, COLOR_TEXT_4);
}


void quantizeLineAction();

void dragTimeLineAction() {
    Rectangle rect = buttonGetRectangle(timeLineDragButton);
    float mx = globalMouseHandler.pos.x-5;
    float tval = floatClip((mx-rect.x)/rect.width, 0, 1);

    double edges=0.15;

    double ctime = globalHandlerGetTime();
    double cdur = globalHandlerGetVisibleDuration();
    //double ltime = globalHandlerGetLineTime();

    
    globalHandlerSetLineTime(ctime+cdur*tval);

    if (tval<edges) {
        if (ctime>0) {
            double speed = (tval/edges)-1.0;
            double offset = 0.035*cdur*speed;
            //globalHandlerSetLineTime(ltime+offset);
            globalHandlerSetTime(ctime+offset);
        } else {
            globalHandlerSetLineTime(ctime+cdur*tval);
        }

    } else if (tval>1-edges) {
        double speed = (tval-1.0+edges)/edges;
        double offset = 0.03*cdur*speed;
        //globalHandlerSetLineTime(ltime+offset);
        globalHandlerSetTime(ctime+offset);
    }
    
    quantizeLineAction();
}



void quantizeLineAction() {
    double ltime = globalHandlerGetLineTime();
    double cdur = globalHandlerGetVisibleDuration();
    double bdur = globalHandlerGetBeatDuration();
    double mdur = globalHandlerGetMeasureDuration();

    if (cdur/mdur>5) {
        double measures = ltime/mdur;
        double quantized = round(measures)*mdur;
        double rem = fabs(ltime-quantized);

        if (rem/cdur<0.02) globalHandlerSetLineTime(quantized);

    } else {
        double beats = ltime/bdur;
        double quantized = round(beats)*bdur;
        double rem = fabs(ltime-quantized);

        if (rem/cdur<0.02) globalHandlerSetLineTime(quantized);
    }
}


static void precomputeTrackCLine() {
    float buttonHeight = 0.7*trackCLineHeight, ypos=controlLineHeight+0.15*trackCLineHeight;
    float space = floatMax(interfaceSpace1, 0.25*trackLeftWidth);
    

    /*  addTrackButton  */

    Rectangle rect = {interfaceSpace1, ypos,  trackLeftWidth-2*space, buttonHeight};
    buttonUpdateRectangle(addTrackButton, rect);
    if (tracksCanCreateNew()) buttonEnable(addTrackButton);
    else buttonDisable(addTrackButton);
    buttonUpdate(addTrackButton, -1);
    
    if (isButtonClicked(addTrackButton)) actionDefer(renderTrackCreateNew);


    /*  timeLineDragButton  */

    rect = (Rectangle){trackLeftWidth, controlLineHeight,  screenSize.x-trackLeftWidth, trackCLineHeight};
    buttonUpdateRectangle(timeLineDragButton, rect);
    buttonUpdate(timeLineDragButton, -1);
    if (!globalHandlerIsPlaying() && isButtonDragged(timeLineDragButton)) actionDefer(dragTimeLineAction);
    else if (!globalHandlerIsPlaying() && isButtonReleased(timeLineDragButton)) actionDefer(quantizeLineAction);


    /*  bottomViewButtons[3]  */

    int tracksNum = projectGetTracksNum(), trackSel=globalHandlerGetSelectedTrack();
    enum keyboard_render_types kbType = globalStateHandlerGetKeyboardType();
    int isSel[3] = {kbType==T_KEYBOARD_NONE, kbType==T_KEYBOARD_HORIZONTAL, kbType==T_KEYBOARD_VERTICAL};
    OnClickFunc actions[3] = {selectBottomViewNone, selectBottomViewHorizontal, selectBottomViewVertical};
    for (int i=0; i<3; i++) {
        Button btn = bottomViewButtons[i];
        Rectangle rect={trackLeftWidth-(interfaceSpace1+buttonHeight)*(3-i), ypos, buttonHeight, buttonHeight};
        if (!i || (tracksNum>0 && trackSel>=0)) buttonEnable(btn);
        else buttonDisable(btn);

        buttonUpdateRectangle(btn, rect);
        int isSelected = isSel[i];
        buttonUpdate(btn, isSelected?1:-1);

        if (!isSelected && isButtonClicked(btn)) actionDefer(actions[i]);
    }
}

void renderTrackCLine() {
    DrawRectangleV((Vector2){0, controlLineHeight}, (Vector2){trackLeftWidth, trackCLineHeight}, COLOR_TRACK_C_LINE_BACKGROUND);

    Rectangle rect = buttonGetRectangle(addTrackButton);
    int isEnabled = isButtonEnabled(addTrackButton);
    float roundness = buttonGetRoundness(addTrackButton);
    float effect = buttonGetEffectValue(addTrackButton);
    Color col1 = {23, 25, 29, 255}, col2={32, 35, 40, 255};
    Color blend1;
    if (isEnabled) blend1 = blendColors(col1, col2, effect);
    else blend1 = blendColors(col1, (Color){0,0,0,255}, 1-0.3*effect);

    DrawRectangleRounded(rect, roundness, 8, blend1);

    Rectangle icRect = {rect.x+interfaceSpace1, controlLineHeight+0.25*trackCLineHeight, 0.5*trackCLineHeight, 0.5*trackCLineHeight};
    renderFontStringAlign(GlobalFonts[0].font, "Add Track", (Vector2){0.5*(icRect.x+icRect.width+rect.x+rect.width), controlLineHeight+0.5*trackCLineHeight}, (Vector2){0.5,0.5}, 0.42*trackCLineHeight, 0, isEnabled?COLOR_TEXT_1:COLOR_TEXT_4);
    iconRerder(T_ICON_ADD, icRect, isEnabled?COLOR_TEXT_1:COLOR_TEXT_4);



    int tracksNum = projectGetTracksNum(), trackSel=globalHandlerGetSelectedTrack();
    enum icon_title icons[3] = {T_ICON_VIEW_NONE, T_ICON_KEYBOARD, T_ICON_VIEW_ROLL};
    for (int i=0; i<3; i++) {
        Button btn = bottomViewButtons[i];
        Rectangle rect=buttonGetRectangle(btn);
        float effect=buttonGetEffectValue(btn);
        Color bkg=blendColors((Color){20,20,20,255}, (Color){35,35,35,255}, effect);
        DrawRectangleRounded(scaleRctangleFromCenter(rect, 0.6+0.4*effect), buttonGetRoundness(btn), 4, bkg);
        Color frg=blendColors((Color){180,180,180,255},(Color){220,220,220,255},effect);
        if (i && (!tracksNum || trackSel<0)) frg=blendColors((Color){100,100,100,255},frg,effect);
        iconRerder(icons[i], scaleRctangleFromCenter(rect, 0.8), frg);
    }

    int ypos = controlLineHeight+trackCLineHeight;
    //DrawRectangleGradientV(0, ypos, trackLeftWidth, interfaceSpace1, (Color){5,5,5,160}, (Color){5,5,5,0});
    //DrawLineEx((Vector2){0, ypos}, (Vector2){trackLeftWidth, ypos}, 2, COLOR_TEXT_4);

    DrawRectangleGradientV(0, ypos, screenSize.x+2, interfaceSpace1, (Color){5,5,5,160}, (Color){5,5,5,0});
    DrawLineEx((Vector2){0, ypos}, (Vector2){screenSize.x+2, ypos}, 2, COLOR_TEXT_4);
}

void createTrackOptionLayout(TrackUI track) {
    if (!track) return;
    if (track->btnList) buttonListFree(track->btnList, 1);

    Rectangle trect = buttonGetRectangle(track->optionsButton);
    Rectangle brect = {trect.x+trect.width+interfaceSpace1, trect.y, buttonList4x5ExampleRect.width, buttonList4x5ExampleRect.height};
    track->btnList = buttonListCreate(brect, 5, 0.15, buttonList4x5ExampleSpacing, 1);
}

void destroyTrackOptionLayout(TrackUI track) {
    if (!track || !(track->btnList)) return;
    buttonListFree(track->btnList, 1);
    track->btnList = NULL;
}


void deleteSelectedTrack() {
    int sel = globalHandlerGetSelectedTrack();
    if (sel<0) return;
    renderTrackDeleteAtIdx(sel);

    int totalTracks = projectGetTracksNum();
    if (sel>=totalTracks) globalHandlerSelectTrack(totalTracks-1);
    else globalHandlerSelectTrack(sel);

    synthPanic();
    scrollToShowSelectedTrack();
}


void changeSelectedTrackColorApproach1() {
    int sel = globalHandlerGetSelectedTrack();
    if (sel<0) return;

    tracks[sel].themeIdx = (tracks[sel].themeIdx+1)%(sizeof(trackThemeColors)/sizeof(Color));
    tracks[sel].theme = trackThemeColors[tracks[sel].themeIdx];
}


void _exportTrackAsMidi() {
    int idx = globalHandlerGetSelectedTrack();
    Track track = trackGetSelectedTrack();
    if (!track) return;

    globalHandlerPause();
    synthPanic();

    const char* trackTitle = trackGetTitle(track);
    if (!trackTitle) trackTitle = "Untitled Track";

    char* title = stringToFileName(trackTitle, 30);
    char* conct = concatenateStrings(title, ".mid");
    free(title);

    const char* path = tinyfd_saveFileDialog("Export MidiLab Track As .MID", conct, 1, (const char *[]){"*.mid"}, "MIDI Format");
    free(conct);

    if (path) {
        int failed = exportTrackAsMidi(path, track);
        if (!failed) destroyTrackOptionLayout(tracks+idx);
    }
}

void _importTrackFromFile() {
    int idx = globalHandlerGetSelectedTrack();
    Track track = trackGetSelectedTrack();
    if (!track) return;

    globalHandlerPause();
    synthPanic();

    const char *path = tinyfd_openFileDialog("Import MidiLab Track", "", 1, (const char *[]){"*.mlt"}, "MidiLab Track", 0);
    if (path) {
        int canReplace = trackCanSafelyReplaceContents(track);
        if (!canReplace) {
            int result = tinyfd_messageBox("Warning", "Are you sure you want to replace the current track?\nYour current project will be lost.", "yesno", "warning", 0);
            if (!result) return;
        }

        printf("Trying to open: %s\n", path);
        int failed = importTrackFrom(path, idx);
        if (!failed) {
            destroyTrackOptionLayout(tracks+idx);
            scrollToShowSelectedTrack();
        }
    }
}

void _exportTrackToFile() {
    int idx = globalHandlerGetSelectedTrack();
    Track track = trackGetSelectedTrack();
    if (!track) return;

    globalHandlerPause();
    synthPanic();

    const char* trackTitle = trackGetTitle(track);
    if (!trackTitle) trackTitle = "Untitled Track";

    char* title = stringToFileName(trackTitle, 30);
    char* conct = concatenateStrings(title, ".mlt");
    free(title);

    const char* path = tinyfd_saveFileDialog("Export MidiLab Track", conct, 1, (const char *[]){"*.mlt"}, "MidiLab Track");
    free(conct);
    
    if (path) {
        int failed = exportTrackTo(path, idx);
        if (!failed) destroyTrackOptionLayout(tracks+idx);
    }
}

void precomputeTrackOptionLayout(TrackUI track) {
    if (!track || !(track->btnList)) return;

    Rectangle trect = buttonGetRectangle(track->optionsButton);
    Rectangle brect = {trect.x+trect.width+interfaceSpace1, trect.y, buttonList4x5ExampleRect.width, buttonList4x5ExampleRect.height};
    float space = 10;
    if (brect.height>trackDivVisiblePosMax-trackDivVisiblePosMin-2*space) brect.y=trackDivVisiblePosMin+space;
    else brect = rectangleMoveToFitInsideRect(brect, (Rectangle){0, trackDivVisiblePosMin+space, screenSize.x, trackDivVisiblePosMax-trackDivVisiblePosMin-2*space});
    buttonListUpdateRect(track->btnList, brect);
    buttonListUpdateSpacing(track->btnList, buttonList4x5ExampleSpacing);
    buttonListUpdate(track->btnList);

    OnClickFunc funcs[] = {changeSelectedTrackColorApproach1, _importTrackFromFile, _exportTrackToFile, _exportTrackAsMidi, deleteSelectedTrack};
    int num = sizeof(funcs)/sizeof(OnClickFunc);
    for (int i=0; i<num; i++) {
        Button btn = buttonListGetButtonAt(track->btnList, i);
        if (funcs[i] && isButtonClicked(btn)) actionDefer(funcs[i]);
    }
    if (buttonListShouldDelete(track->btnList)) destroyTrackOptionLayout(track);
}


void renderTrackOptionLayoutButton(Button btn, const char* text, Vector2 textAlign, Vector2 textOffset, float textSize, Color theme, int type) {
    if (!btn) return;
    Color col={30, 32, 38, 255};
    float effect = buttonGetEffectValue(btn);
    Color blend1 = blendColors(col, type?COLOR_RED_DELETE_1:theme, type?(0.15+0.2*effect):(0.35*effect));
    Rectangle rect = buttonGetRectangle(btn);
    DrawRectangleRounded(rect, buttonGetRoundness(btn), 8, blend1);
    Vector2 tarPos = lerpVector2_vec((Vector2){rect.x, rect.y}, (Vector2){rect.x+rect.width, rect.y+rect.height}, textAlign);
    renderFontStringAlign(GlobalFonts[0].font, text, Vector2Add(tarPos, textOffset), textAlign, textSize, 0, COLOR_TEXT_1);
}

void renderTrackOptionLayout(TrackUI track) {
    if (!track || !(track->btnList)) return;

    Rectangle brect = buttonListGetRect(track->btnList);
    Color col1 = {20, 21, 23, 255};
    float roundness = buttonListGetRoundness(track->btnList);
    DrawRectangleRoundedLinesEx(brect, roundness, 8, 8, (Color){2, 2, 2, 100});
    DrawRectangleRounded(brect, roundness, 8, col1);
    const char* texts[] = {"Change Color", "Import Track", "Export Track", "Export Midi", "Delete Track"};
    int num = buttonListGetNum(track->btnList);
    for (int i=0; i<num; i++) renderTrackOptionLayoutButton(buttonListGetButtonAt(track->btnList, i), texts[i], (Vector2){0, 0.5}, (Vector2){10, 0}, brect.height*0.08, track->theme, i==num-1);
    
    DrawRectangleRoundedLinesEx(brect, roundness, 8, 2, blendColors(col1, track->theme, 0.35));
}


void actionSelectLeftTrack() {
    int totalTracks = projectGetTracksNum();
    for (int i=0; i<totalTracks; i++) {
        if (isButtonClicked(tracks[i].base)) {
            globalHandlerSelectTrack(i);
            globalHandlerSetKeyboardType(T_KEYBOARD_HORIZONTAL);
            scrollToShowSelectedTrack();
            break;
        }
    }
}

void actionSelectRightTrack() {
    int totalTracks = projectGetTracksNum();
    for (int i=0; i<totalTracks; i++) {
        if (isButtonClicked(tracks[i].rightBase)) {
            globalHandlerSelectTrack(i);
            globalHandlerSetKeyboardType(T_KEYBOARD_VERTICAL);
            scrollToShowSelectedTrack();
            break;
        }
    }
}

void renderTrackMoveToIndex(int old, int new) {
    int totalTracks = projectGetTracksNum();
    if (old<0 || old>=totalTracks || new<0 || new>=totalTracks) return;
    if (old==new) return;
    if (trackMoveToIndex(trackGetAtIdx(old), new)!=new) return;

    struct track_control_ui strack = tracks[old];
    if (old<new) {
        memmove(tracks+old, tracks+old+1, (new-old)*sizeof(struct track_control_ui));
    } else if (old>new) {
        memmove(tracks+new+1, tracks+new, (old-new)*sizeof(struct track_control_ui));
    }
    //printf("renderTrackMoveToIndex: old=%d, new=%d\n", old, new);
    tracks[new] = strack;
    globalHandlerSelectTrack(new);
}

void clipTrackScrollTarget() {
    int totalTracks = projectGetTracksNum();
    float totalHeight = totalTracks*trackHeight+10;
    if (trackScrollYtarget>totalHeight-trackDivTargetHeight) trackScrollYtarget = totalHeight-trackDivTargetHeight; // totalHeight>trackDivTargetHeight && 
    if (trackScrollYtarget<0) trackScrollYtarget=0;
}

void precomputeMovingTrackLeft(int idx) {
    int totalTracks = projectGetTracksNum();
    if (idx<0 || idx>=totalTracks) return;
    TrackUI track = tracks+idx;
    if (!(track->moving)) return;

    
    float my=globalMouseHandler.pos.y;
    float vy=floatClip((my-trackDivLeftRect.y)/trackDivLeftRect.height, 0, 1);
    double edgeY=0.25;

    if (vy<edgeY) {
        double speed = (vy/edgeY)-1.0;
        trackScrollYtarget += 10*speed;
    } else if (vy>1-edgeY) {
        double speed = (edgeY+vy-1.0)/edgeY;
        trackScrollYtarget += 10*speed;
    }
    clipTrackScrollTarget();
    
    float ny = (trackDivLeftRect.y+trackDivLeftRect.height*vy-controlLineHeight-5-trackCLineHeight+trackScrollY)/trackHeight-0.5;
    int nidx = intClip(round(ny), 0, totalTracks-1);
    
    track->y = ny;
    if (idx!=nidx) renderTrackMoveToIndex(idx, nidx);
}


void precomputeTrackLeft(int idx) {
    int isSelected=(globalHandlerGetSelectedTrack()==idx), disableHover=(openLayout || !CheckCollisionPointRec(globalMouseHandler.pos, trackDivLeftRect));
    
    TrackUI track = tracks+idx;
    Track trackAbstr = trackGetAtIdx(idx);
    track->track = trackAbstr;
    if (!(track->moving)) track->y += 0.2*((float)idx-track->y);
    track->shadow += 0.2*(track->moving-track->shadow);
    
    float ypos = controlLineHeight+5+trackCLineHeight + track->y*trackHeight-trackScrollY;
    Rectangle baseRect = {interfaceSpace1, ypos+interfaceSpace1*0.5, trackLeftWidth-2*interfaceSpace1, trackHeight-interfaceSpace1};
    buttonUpdateRectangle(track->base, baseRect);
    if (disableHover) buttonDisableHover(track->base);
    else buttonEnableHover(track->base);
    buttonUpdate(track->base, isSelected?1:-1);

    if (isButtonClicked(track->base) && (!isSelected || globalStateHandlerGetKeyboardType()!=T_KEYBOARD_HORIZONTAL)) {
        actionDefer(actionSelectLeftTrack);
    }

    // Right track
    Rectangle rightRect = {trackLeftWidth+interfaceSpace1, ypos+interfaceSpace1*0.5, screenSize.x-trackLeftWidth-4*interfaceSpace1, trackHeight-interfaceSpace1};
    buttonUpdateRectangle(track->rightBase, rightRect);
    if (!openLayout && CheckCollisionPointRec(globalMouseHandler.pos, trackDivRightRect)) buttonEnableHover(track->rightBase);
    else buttonDisableHover(track->rightBase);
    buttonUpdate(track->rightBase, isSelected?1:-1);
    if (isButtonClicked(track->rightBase) && (!isSelected || globalStateHandlerGetKeyboardType()!=T_KEYBOARD_VERTICAL)) {
        actionDefer(actionSelectRightTrack);
    }

    int allowHoverLeft = (isSelected && !openLayout && globalStateHandlerGetKeyboardType()==T_KEYBOARD_HORIZONTAL);

    float targetEffect = isSelected?1:(0.5*(isButtonHovered(track->base) || isButtonHovered(track->rightBase)));
    track->effect += 0.15*(targetEffect-track->effect);


    // Textbox
    Rectangle tbxRect = {baseRect.x+0.6*baseRect.height-5, baseRect.y+0.15*baseRect.height, baseRect.width-1.2*baseRect.height+10, 0.32*baseRect.height};
    if (allowHoverLeft) textboxEnable(track->textbox);
    else textboxDisable(track->textbox);

    textboxUpdateRectangle(track->textbox, tbxRect);
    if (disableHover) textboxDisableHover(track->textbox);
    else textboxEnableHover(track->textbox);
    textboxUpdate(track->textbox, isSelected?-1:0);

    if (isTextboxFocused(track->textbox)) textboxUpdateText(track->textbox);
    if (isTextboxJustUnfocused(track->textbox)) trackSetTitle(trackAbstr, textboxGetText(track->textbox));


    // Options (Button)
    Rectangle opRect = scaleRctangleFromCenter((Rectangle){baseRect.x+baseRect.width-0.8*baseRect.height, baseRect.y, baseRect.height, baseRect.height}, 0.32);
    if (isSelected) buttonEnable(track->optionsButton);
    else buttonDisable(track->optionsButton);

    buttonUpdateRectangle(track->optionsButton, opRect);
    if (disableHover) buttonDisableHover(track->optionsButton);
    else buttonEnableHover(track->optionsButton);
    buttonUpdate(track->optionsButton, isSelected?(track->btnList?1:-1):0);

    if (isSelected && !(track->btnList) && isButtonClicked(track->optionsButton)) {
        createTrackOptionLayout(track);
    }

    if (track->btnList) {
        if (!isSelected) destroyTrackOptionLayout(track);
        else precomputeTrackOptionLayout(track);
    }


    // Slider (Volume)
    Rectangle sldRect = {baseRect.x+0.6*baseRect.height, baseRect.y+0.67*baseRect.height, baseRect.width-1.6*baseRect.height, 0.175*baseRect.height};
    if (allowHoverLeft) sliderEnable(track->slider);
    else sliderDisable(track->slider);

    sliderUpdateRectangle(track->slider, sldRect);
    if (disableHover) sliderDisableHover(track->slider);
    else sliderEnableHover(track->slider);
    sliderUpdate(track->slider, isSelected?-1:0);

    if (isSelected && isSliderDragged(track->slider)) {
        float val=sliderUpdateValueCommonHorizontal(track->slider);
        trackSetVelocity(trackAbstr, val);
    } else sliderUpdateSlideValue(track->slider, trackGetVelocity(trackAbstr));

    // Icon (Program/Instrument)
    track->icon = midiGetProgramTypeIcon(trackGetProgram(track->track));


    // Move button
    Rectangle mvRect = scaleRctangleFromCenter((Rectangle){baseRect.x-0.2*baseRect.height, baseRect.y, baseRect.height, baseRect.height}, 0.45);
    if (isSelected) buttonEnable(track->move);
    else buttonDisable(track->move);

    buttonUpdateRectangle(track->move, mvRect);
    if (disableHover) buttonDisableHover(track->move);
    else buttonEnableHover(track->move);
    buttonUpdate(track->move, isSelected?(track->moving?1:-1):0);

    if (isSelected && isButtonDragged(track->move)) track->moving = 1;
    else if (isButtonReleased(track->move)) {
        track->moving = 0;
        scrollToShowSelectedTrack();
    } else track->moving = 0;
}


void renderTrackLeft(int idx, int skipMoving) {
    int isSelected = (globalHandlerGetSelectedTrack()==idx);
    TrackUI track = tracks+idx;
    if (isSelected && skipMoving) return;
    
    Rectangle baseRect = buttonGetRectangle(track->base);
    Rectangle rightRect = buttonGetRectangle(track->rightBase);
    float shadowPxh = baseRect.height*0.06*track->shadow;
    if (baseRect.y-shadowPxh>trackDivVisiblePosMax || baseRect.y+baseRect.height+shadowPxh<trackDivVisiblePosMin) return;

    float ypos = controlLineHeight+5+trackCLineHeight + track->y*trackHeight-trackScrollY;
    float effect = track->effect;

    if (shadowPxh>0.1) {
        Vector2 extraSh = {shadowPxh, shadowPxh};
        Color cl = blendColors((Color){0,0,0,255}, track->theme, 0.08*track->shadow);
        cl.a = (unsigned char)(150.0*track->shadow);
        Rectangle nrect = rectangleIncrease(baseRect, extraSh,extraSh);
        DrawRectangleRounded(nrect, getRoundnessForRoundedRectangleTransformation(baseRect, nrect, 0.2, shadowPxh), 8, cl);
        nrect = rectangleIncrease(rightRect, extraSh,extraSh);
        DrawRectangleRounded(nrect, getRoundnessForRoundedRectangleTransformation(rightRect, nrect, buttonGetRoundness(track->rightBase), shadowPxh), 8, cl);
    }

    Color col1={25,27,30,255}; //col2={36,40,47,255};
    Color blendBackground = blendColors(col1, track->theme, effect*0.1); //blendColors(col1, col2, effect);
    DrawRectangleRounded(baseRect, 0.2, 8, blendBackground);
    if (effect>1e-3) {
        Color tcol=track->theme; tcol.a=(unsigned char)lerp(0, 255, effect);
        float ty=ypos+0.5*trackHeight, tx=baseRect.x+baseRect.width-0.5*interfaceSpace1, tw=interfaceSpace1, th=trigInterpolation(0, trackHeight-2*interfaceSpace1, effect);
        renderRoundedRectangleCentered((Vector2){tx, ty}, (Vector2){tw, th}, tcol, 1, 8);
    }
    if (!isSelected && isButtonClicked(track->base)) globalHandlerSelectTrack(idx);

    float rightEffect = buttonGetEffectValue(track->rightBase);
    Color rightBg = blendColors(blendBackground, track->theme, 0.2+0.2*rightEffect);
    DrawRectangleRounded(rightRect, buttonGetRoundness(track->rightBase), 8, rightBg);

    if (track->preview.id) {
        //float tbpx = 0.1;
        float pixOffset = getRadiusForRoundedRectangle(rightRect, buttonGetRoundness(track->rightBase));
        double beatDur = globalHandlerGetBeatDuration(), pixPerBeat = _trackPreviewPixelsPerBeat();
        float x=pixPerBeat*globalHandlerGetTime()/beatDur, w=pixPerBeat*globalHandlerGetVisibleDuration()*(rightRect.width/(trackDivRightRect.width-rightRect.x+trackDivRightRect.x))/beatDur;
        float tx=floatMin(x, track->preview.texture.width);
        float tw=floatMin(x+w, track->preview.texture.width)-tx;
        Rectangle source = {tx, 0, tw, track->preview.texture.height};
        Rectangle dest = {rightRect.x, rightRect.y+pixOffset, rightRect.width*tw/(x+w-tx), rightRect.height-2*pixOffset};
        if (x<track->preview.texture.width) DrawTexturePro(track->preview.texture, source, dest, (Vector2){0,0}, 0, blendColors(track->theme, (Color){255,255,255,255}, 0.2+0.3*rightEffect));
    }
    

    // Textbox
    Rectangle tbxRect = textboxGetRectangle(track->textbox);
    int focusedTbx = isTextboxFocused(track->textbox);
    float tbxEffect = textboxGetEffectValue(track->textbox);
    
    Color tbxbg = blendColors(blendBackground, (Color){45,49,57,255}, tbxEffect);
    DrawRectangleRounded(tbxRect, textboxGetRoundness(track->textbox), 8, tbxbg);

    const char* trackTitle = textboxGetText(track->textbox);

    float fontSize = 0.7*tbxRect.height;
    Vector2 textPos = (Vector2){tbxRect.x+5, tbxRect.y+0.5*tbxRect.height};
    if (*trackTitle) renderFontStringAlign(GlobalFonts[0].font, trackTitle, textPos, (Vector2){0, 0.5}, fontSize, 0, focusedTbx?COLOR_TEXT_1:COLOR_TEXT_3);
    else if (!focusedTbx) renderFontStringAlign(GlobalFonts[1].font, TRACK_TITLE_PLACEHOLDER, textPos, (Vector2){0, 0.5}, fontSize*0.9, 0, COLOR_TEXT_4);


    if (focusedTbx && textboxGetCursorIdx(track->textbox)>=0) {        
        Color col = COLOR_TEXT_1;
        col.a = (unsigned char)trigInterpolation(255*effect, 0, fmod(1.1*GetTime(), 1.0));

        if (*trackTitle) {
            char* partBeforeCursor = textboxGetTextBeforeCursor(track->textbox);
            Vector2 cursorOffset = MeasureTextEx(GlobalFonts[0].font, partBeforeCursor, fontSize, 0);
            free(partBeforeCursor);
            textPos.x += cursorOffset.x;
        }

        DrawLineEx((Vector2){textPos.x, textPos.y-0.25*tbxRect.height}, (Vector2){textPos.x, textPos.y+0.25*tbxRect.height}, 1, col);
    }


    // Slider
    float scaleDown = 0.4;
    Rectangle sldRect = sliderGetRectangle(track->slider); sldRect=scaleRctangleFromCenterV(sldRect, (Vector2){1, scaleDown});
    float sldEffect = sliderGetEffectValue(track->slider);
    float sliderVal = sliderGetSlideValue(track->slider);
    
    Color sldbg = {15, 17, 22, 255};
    DrawRectangleRounded(sldRect, 1, 8, sldbg);
    Color sldfg = blendColors(track->theme, (Color){255,255,255,255}, 0.3*sldEffect);
    DrawRectangleRounded(scaleRctangleFromCenterV(sliderGetRectangleValueCommon(track->slider), (Vector2){1, scaleDown}), 1, 8, sldfg);
    if (sldEffect>1e-3) {
        DrawCircleV(sliderGetPosValueCommon(track->slider), lerp(0, 0.75, sldEffect)*sldRect.height, blendColors(track->theme, (Color){255,255,255,255}, 0.6*sldEffect));
    }

    Rectangle rct = centerRectangle((Vector2){sldRect.x+sldRect.width+4*sldRect.height, sldRect.y+0.5*sldRect.height}, (Vector2){4*sldRect.height, 4*sldRect.height}); //{sldRect.x+sldRect.width+0.08*baseRect.height, sldRect.y-1.5*sldRect.height, sldRect.height*3}
    enum icon_title icon = (sliderVal>0.8?T_ICON_VOLUME_MAX:(sliderVal>0.4?T_ICON_VOLUME_MID:(sliderVal>0?T_ICON_VOLUME_MIN:T_ICON_VOLUME_NONE)));
    iconRerder(icon, rct, sldfg);


    if (track->icon != T_ICON_END) {
        Rectangle icRect = scaleRctangleFromCenter((Rectangle){baseRect.x-0.2*baseRect.height, baseRect.y, baseRect.height, baseRect.height}, 0.22);
        if (effect>0.501) {
            Color tcl = track->theme;
            tcl.a = (unsigned char)(56*(effect-0.5));
            tcl = blendColors(tcl, (Color){200,200,200,tcl.a}, 0.04*(effect-0.5));
            //DrawCircleV(getRectangleCenter(icRect), icRect.width*1.15*2.0*(effect-0.5), tcl);
            DrawRectangleRounded(buttonGetRectangle(track->move), 1.0-(effect-0.5), 8, tcl);
        }
        iconRerder(track->icon, scaleRctangleFromCenter(icRect, 2*normalizeProgramTypeIcon(track->icon)), track->theme);
    }


    // Options button
    Rectangle opRect = buttonGetRectangle(track->optionsButton);
    float opEffect = buttonGetEffectValue(track->optionsButton);
    Color opbg = blendColors(blendBackground, (Color){45,49,57,255}, opEffect);
    if (opEffect>1e-3) DrawRectangleRounded(scaleRctangleFromCenter(opRect, lerp(0.3, 1, opEffect)), buttonGetRoundness(track->optionsButton), 8, opbg);
    iconRerder(T_ICON_OPTIONS, scaleRctangleFromCenter(opRect, 0.75*lerp(0.9, 0.95, opEffect)), blendColors(track->theme, COLOR_TEXT_1, opEffect));
}

void renderMovingTrack() {
    int idx = globalHandlerGetSelectedTrack();
    if (idx<0 || idx>=projectGetTracksNum()) return;
    //TrackUI track = tracks+idx;
    renderTrackLeft(idx, 0);
}

void renderActualTracksLeft() {
    int totalTracks = projectGetTracksNum();

    if (totalTracks){
        for (int i=0; i<totalTracks; i++) renderTrackLeft(i, 1);
        renderMovingTrack();

    } else {
        float ypos = controlLineHeight+5+trackCLineHeight;
        renderFontStringAlign(GlobalFonts[0].font, "No Available Tracks", (Vector2){0.5*trackLeftWidth, ypos+0.5*trackHeight}, (Vector2){0.5,0.5}, 0.48*trackCLineHeight, 0, COLOR_TEXT_4);
        float y1=ypos+interfaceSpace1*0.5, y2=ypos+trackHeight-interfaceSpace1;
        DrawLineDashed((Vector2){interfaceSpace1, y1}, (Vector2){trackLeftWidth-interfaceSpace1, y1}, 8, 6, COLOR_TEXT_4);
        DrawLineDashed((Vector2){trackLeftWidth-interfaceSpace1, y1}, (Vector2){trackLeftWidth-interfaceSpace1, y2}, 8, 6, COLOR_TEXT_4);
        DrawLineDashed((Vector2){trackLeftWidth-interfaceSpace1, y2}, (Vector2){interfaceSpace1, y2}, 8, 6, COLOR_TEXT_4);
        DrawLineDashed((Vector2){interfaceSpace1, y2}, (Vector2){interfaceSpace1, y1}, 8, 6, COLOR_TEXT_4);
    }
}


void precomputeTrackSlider() {
    if (!trackSlider) return;

    int totalTracks = projectGetTracksNum();
    float totalHeight = totalTracks*trackHeight+10;

    Rectangle rect = {screenSize.x-2*interfaceSpace1, trackDivLeftRect.y+interfaceSpace1, interfaceSpace1, trackDivLeftRect.height-2*interfaceSpace1};
    sliderUpdateRectangle(trackSlider, rect);
    if (trackDivTargetHeight>=totalHeight) sliderDisable(trackSlider);
    else {
        sliderEnable(trackSlider);
        float val = trackScrollY/(totalHeight-trackDivTargetHeight);
        sliderUpdateSlideValue(trackSlider, val);
    }
    sliderUpdate(trackSlider, -1);

    if (isSliderDragged(trackSlider)) {
        float th = rect.height*trackDivTargetHeight/totalHeight;
        float mposClip = floatClip(globalMouseHandler.pos.y, rect.y+0.5*th, rect.y+rect.height-0.5*th);
        float nval = (mposClip-rect.y-0.5*th)/(rect.height-th);
        trackScrollYtarget = nval*(totalHeight-trackDivTargetHeight);
    }

}

void renderTrackSlider() {
    if (!trackSlider || isSliderDisabled(trackSlider)) return;
    
    int totalTracks = projectGetTracksNum();
    float totalHeight = totalTracks*trackHeight+10;

    Rectangle rect = sliderGetRectangle(trackSlider);
    float roundness = sliderGetRoundness(trackSlider);
    float effect = sliderGetEffectValue(trackSlider);
    float val = sliderGetSlideValue(trackSlider);
    Color col = blendColors((Color){100,100,100,120}, (Color){180,180,180,160}, effect);

    float th = rect.height*trackDivTargetHeight/totalHeight;
    Rectangle newRect = {rect.x, rect.y+(rect.height-th)*val, rect.width, th};
    DrawRectangleRounded(newRect, roundness, 4, col);

}

static void _eventListenerForTrackTextures() {
    int selTr = globalHandlerGetSelectedTrack();
    int totalTracks = projectGetTracksNum();
    if (shouldUpdateAllTextures) {
        printf("Run `_eventListenerForTrackTextures`: totalTracks=%d\n", totalTracks);
        shouldUpdateAllTextures=0;
        for (int i=0; i<totalTracks; i++) trackUIgenerateTrackPreview(i);
    } else if (selTr!=previouslySelectedTrack) {
        if (previouslySelectedTrack<totalTracks && previouslySelectedTrack>=0) trackUIgenerateTrackPreview(previouslySelectedTrack);
        if (selTr>=0 && selTr<totalTracks) trackUIgenerateTrackPreview(selTr);
    }
    previouslySelectedTrack=selTr;
}

void order1PrecomputeTracksLeft() {
    trackCLineHeight = floatMax(controlLineHeight*0.7, 30);
    trackHeight = floatMax(85, screenSize.y*0.085);
    trackLeftWidth = floatMax(280, 0.2*screenSize.x);
    int totalTracks = projectGetTracksNum();
    openLayout=0;
    for (int i=0; i<totalTracks; i++) if (tracks[i].btnList) {openLayout=1; break;}

    _eventListenerForTrackTextures();
}

void order2PrecomputeTracksLeft() {
    trackDivTargetHeight = screenSize.y-controlLineHeight-trackCLineHeight-bottomHalfHeight;
    trackDivVisiblePosMin=controlLineHeight+trackCLineHeight, trackDivVisiblePosMax=screenSize.y-bottomHalfHeight;
    trackDivLeftRect = (Rectangle){0, trackDivVisiblePosMin, trackLeftWidth, trackDivTargetHeight};
    trackDivRightRect = (Rectangle){trackLeftWidth, trackDivVisiblePosMin, screenSize.x-trackLeftWidth, trackDivTargetHeight};
    trackDivFullRect = (Rectangle){0, trackDivVisiblePosMin, screenSize.x, trackDivTargetHeight};

    enum keyboard_render_types kbType = globalStateHandlerGetKeyboardType();
    Rectangle zoomDivRect = (Rectangle){trackLeftWidth, controlLineHeight, screenSize.x-trackLeftWidth, (kbType==T_KEYBOARD_VERTICAL)?(screenSize.y-controlLineHeight):trackDivTargetHeight+trackCLineHeight};

    //int mouseInDiv = !UIisHoveringOverLayout() && !UIisInTextInput() && CheckCollisionPointRec(globalMouseHandler.pos, trackDivFullRect);
    int totalTracks = projectGetTracksNum();
    int scrolled =  !UIisHoveringOverLayout() && !UIisInTextInput() && globalMouseHandler.scroll!=0;
    //float totalHeight = totalTracks*trackHeight+10;
    int controlDown = IsKeyDown(KEY_LEFT_CONTROL);

    if (controlDown && !UIisHoveringOverLayout() && !UIisInTextInput()) {
        if (scrolled && CheckCollisionPointRec(globalMouseHandler.pos, zoomDivRect)) {
            double visDur = globalHandlerGetVisibleDuration();
            visDur *= pow(2, -0.075*globalMouseHandler.scroll);
            globalHandlerSetVisibleDuration(visDur);
        } else {
            double visDur = globalHandlerGetVisibleDuration();
            int d = IsKeyDown(KEY_KP_ADD)-IsKeyDown(KEY_KP_SUBTRACT);
            visDur *= pow(2, -0.075*d);
            globalHandlerSetVisibleDuration(visDur);
        }
        if (IsKeyPressed(KEY_S)) actionDefer(exportProjectByCtrlS);
    }

    if (!controlDown && scrolled) {
        if (CheckCollisionPointRec(globalMouseHandler.pos, trackDivFullRect)) {
            trackScrollYtarget -= 0.5*trackHeight*globalMouseHandler.scroll;
        }
    }

    int dx = IsKeyPressed(KEY_RIGHT)-IsKeyPressed(KEY_LEFT);
    if (dx && !UIisHoveringOverLayout() && !UIisInTextInput() && !globalHandlerIsPlaying()) {
        enum keyboard_render_types kbt = globalStateHandlerGetKeyboardType();
        if ((kbt==T_KEYBOARD_NONE || kbt==T_KEYBOARD_VERTICAL) && !globalMouseHandler.down && !globalMouseHandler.rightClickPressed) {
            if (dx>0) globalHandlerSetToNextMeasure();
            else if (dx<0) globalHandlerSetToPreviousMeasure();
        }
    }

    
    precomputeTrackCLine();
    precomputeMovingTrackLeft(globalHandlerGetSelectedTrack());
    clipTrackScrollTarget();
    trackScrollY += 0.18*(trackScrollYtarget-trackScrollY);
    
    for (int i=0; i<totalTracks; i++) precomputeTrackLeft(i);

    precomputeTrackSlider();
}



void renderTracksLeftLayoutsIfAny() {
    int selected = globalHandlerGetSelectedTrack();

    if (selected>=0 && tracks[selected].btnList) renderTrackOptionLayout(tracks+selected);
}






void renderMeasuresTCLine() {
    double visDur = globalHandlerGetVisibleDuration();
    double ctime = globalHandlerGetTime();
    double beatDur = globalHandlerGetBeatDuration();
    double measureDur = globalHandlerGetMeasureDuration();
    int beatsInMeasure = globalHandlerGetBeatsInMeasure();

    double targetMeasures = globalHandlerDurationToMeasures(visDur);

    float startX=trackLeftWidth+interfaceSpace1, totalWidth=screenSize.x-trackLeftWidth-interfaceSpace1; float measureWidth=totalWidth/targetMeasures;
    float beatWidth = measureWidth/beatsInMeasure, clipErrorRange=50;
    int measureTextSkips=1, measureSkips=1, beatSkips=1, subBeats=0, beatTextSkips=1;

    float minMeasuresTextNSkipped=120, minMeasuresNSkipped=10, minBeatNSkipped=10, minBeatsTextNSkipped=100;

    
    if (measureWidth<minMeasuresNSkipped) measureSkips = ceil(minMeasuresNSkipped/measureWidth);
    if (measureWidth<minMeasuresTextNSkipped) {
        measureTextSkips = ceil(minMeasuresTextNSkipped/measureWidth);

        for (int i=measureTextSkips; i>=1; i--) {
            if (i%measureSkips==0) {measureTextSkips=i; break;}
        }
    }

    if (beatWidth<minBeatNSkipped) {
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
    if (beatWidth<minBeatsTextNSkipped) {
        beatTextSkips = ceil(minBeatsTextNSkipped/beatWidth);
        if (beatTextSkips>=beatsInMeasure) beatTextSkips=beatsInMeasure;
        else {
            for (int i=beatTextSkips; i>=1; i--) {
                if (beatsInMeasure%i==0 && i%beatSkips==0) {beatTextSkips=i; break;}
            }
        }
    }

    if (beatSkips==1) {
        subBeats=(int)round(log2(beatWidth/minBeatNSkipped));
        if (subBeats<0) subBeats=0;
        else subBeats=(1<<subBeats);
    }
    
    
    timelineMeasureSkipsTop=measureSkips;
    timelineMeasureSkipsBottom=measureTextSkips;
    timelineBeatsSkipsTop=beatSkips;
    

    
    float y1 = controlLineHeight+0.35*trackCLineHeight;
    float y2 = controlLineHeight+0.7*trackCLineHeight;
    float y3 = controlLineHeight+trackCLineHeight;
    float y4 = controlLineHeight+0.77*trackCLineHeight;
    float y5 = controlLineHeight+0.84*trackCLineHeight;

    int startMeasure = floor(globalHandlerDurationToMeasures(ctime));
    startMeasure = startMeasure/measureTextSkips; startMeasure=measureTextSkips*startMeasure;
    double start = globalHandlerMeasuresToDuration(startMeasure)-ctime;
    for (int i=0; i<targetMeasures+measureTextSkips+1; i+=measureTextSkips) {
        float posX = startX+totalWidth*(start+measureDur*i)/visDur;
        
        if (startX+totalWidth*(start+measureDur*(i+measureTextSkips))/visDur<startX-clipErrorRange || posX>=screenSize.x+clipErrorRange) continue;

        DrawLineEx((Vector2){posX-1, y2}, (Vector2){posX-1, y3}, 2.0, COLOR_TEXT_1);
        renderFontStringAlign(GlobalFonts[0].font, TextFormat("%d", startMeasure+i+1), (Vector2){posX, y1}, (Vector2){0.5, 0.5}, 0.4*trackCLineHeight, 0, COLOR_TEXT_1);


        if (subBeats) {
            int totalSubBeats = measureTextSkips*beatsInMeasure*subBeats;
            for (int j=1; j<totalSubBeats; j++) {
                posX = startX+totalWidth*(start+beatDur*(beatsInMeasure*i+j/(float)subBeats))/visDur;
                if (posX<startX-clipErrorRange || posX>=screenSize.x+clipErrorRange) continue;
                if (j%(beatsInMeasure*subBeats)==0) {
                    // Measure
                    DrawLineEx((Vector2){posX-1, y4}, (Vector2){posX-1, y3}, 2.0, COLOR_TEXT_3);
                    
                } else if (j%subBeats==0) {
                    // Beat
                    if ((j/subBeats)%beatTextSkips==0) renderFontStringAlign(GlobalFonts[0].font, TextFormat("%d.%d", startMeasure+i+1, (j/subBeats)%beatsInMeasure), (Vector2){posX, y1}, (Vector2){0.5, 0.5}, 0.35*trackCLineHeight, 0, COLOR_TEXT_3);
                    DrawLineEx((Vector2){posX, y4}, (Vector2){posX, y3}, 1.0, COLOR_TEXT_4);
                } else {
                    // SubBeat
                    DrawLineEx((Vector2){posX, y5}, (Vector2){posX, y3}, 1.0, COLOR_TEXT_4);
                }
            }
        } else {
            for (int j=beatSkips; j<measureTextSkips*beatsInMeasure; j+=beatSkips) {
                posX = startX+totalWidth*(start+beatDur*(beatsInMeasure*i+j))/visDur;
                if (posX<startX-clipErrorRange || posX>=screenSize.x+clipErrorRange) continue;
                if (j%beatsInMeasure) {
                    // Beat
                    if (j%beatTextSkips==0) renderFontStringAlign(GlobalFonts[0].font, TextFormat("%d.%d", startMeasure+i+1, j%beatsInMeasure), (Vector2){posX, y1}, (Vector2){0.5, 0.5}, 0.35*trackCLineHeight, 0, COLOR_TEXT_3);
                    DrawLineEx((Vector2){posX, y4}, (Vector2){posX, y3}, 1.0, COLOR_TEXT_4);
                } else if ((j/beatsInMeasure)%measureSkips==0) {
                    // Measure
                    DrawLineEx((Vector2){posX-1, y4}, (Vector2){posX-1, y3}, 2.0, COLOR_TEXT_3);
                }
            }
        }
    }


}




void renderTracksTimeLine() {
    if (!globalHandlerIsTimeLineShown()) return;

    double visDur = globalHandlerGetVisibleDuration();
    double ctime = globalHandlerGetTime();
    double ltime = globalHandlerGetLineTime();

    float startX = trackLeftWidth+interfaceSpace1, totalWidth=screenSize.x-trackLeftWidth-interfaceSpace1, y1=trackCLineHeight+controlLineHeight, y2=screenSize.y, radius=0.08*trackCLineHeight;
    float x = startX+totalWidth/visDur*(ltime-ctime);
    y1-=radius;

    if (x<screenSize.x+radius+5 && x>startX-radius-5) {
        Color tc = COLOR_TEXT_1;
        tc.a = 120;
        float sz = 1+0.001*screenSize.x;

        float effect = buttonGetEffectValue(timeLineDragButton);

        DrawLineEx((Vector2){x, y1}, (Vector2){x, y2}, 1+sz*effect, tc);
        DrawCircleV((Vector2){x, y1}, radius-1+sz*effect, tc);

        Color col = blendColors(COLOR_TEXT_1, COLOR_KEYBOARD_H_KEY_SELECTED_WHITE_HOVERED, effect);

        DrawLineEx((Vector2){x, y1}, (Vector2){x, y2}, 2, col);
        DrawCircleV((Vector2){x, y1}, radius, col);
    }
}



void renderTracksLeft() {
    
    renderActualTracksLeft();
    renderTrackSlider();
    renderTrackCLineBackground();
    renderMeasuresTCLine();
    renderTrackCLine();
    renderTracksTimeLine();
    
}






