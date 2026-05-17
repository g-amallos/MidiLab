#include <interface.h>
#include <backend.h>
#include <raymath.h>
#include <colors.h>
#include <utils.h>
#include <ui.h>
#include <stdlib.h>
#include <stdio.h>
#include <images.h>
#include <handler.h>
#include <math.h>


#define TRACK_TITLE_PLACEHOLDER "Track Title"


float trackHeight=0, trackLeftWidth=0, trackCLineHeight=0, trackDivTargetHeight=0, trackScrollY=0, trackScrollYtarget=0, trackDivVisiblePosMin=0, trackDivVisiblePosMax=0;
Rectangle trackDivLeftRect = {0,0,0,0};
Color trackThemeColors[7], programTypeColors[MPT_END];
Button addTrackButton=NULL;


typedef struct track_control_ui {
    Track track;
    Button base;
    Color theme;
    int themeIdx;
    float effect;
    enum icon_title icon;
    Textbox textbox;
    Slider slider;
    Button optionsButton;
    ButtonList btnList;
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

    addTrackButton = buttonCreate((Rectangle){20,20,20,20}, 0.25);
    initProgramTypeColors();
}


void renderTracksLeftClose() {
    if (addTrackButton) buttonFree(addTrackButton);
    addTrackButton = NULL;

    if (tracks) {
        int totalTracks = projectGetTracksNum();
        for (int i=0; i<totalTracks; i++) {
            if (tracks[i].btnList) buttonListFree(tracks[i].btnList, 1);
            buttonFree(tracks[i].optionsButton);
            textboxFree(tracks[i].textbox);
            buttonFree(tracks[i].base);
            sliderFree(tracks[i].slider);
        }
        free(tracks);
        tracks=NULL;
    }



}

void customizeNewTrackUI(TrackUI tr) {
    if (!tr) return;
    Rectangle rect = (Rectangle){20, 20, 20, 20};
    tr->base = buttonCreate(rect, 0.2);
    int cols = sizeof(trackThemeColors)/sizeof(Color);
    tr->themeIdx = GetRandomValue(0, cols-1);
    tr->theme = trackThemeColors[tr->themeIdx];
    tr->icon = T_ICON_PIANO;
    tr->textbox = textboxCreate(rect, 0.3, T_IN_STRING, 22);
    tr->optionsButton = buttonCreate(rect, 0.2);
    tr->btnList = NULL;
    tr->slider = sliderCreate(rect, 1);
    sliderUpdateCursorOnHover(tr->slider, MOUSE_CURSOR_RESIZE_EW); 
    tr->effect = 0;
}

void renderTrackCreateNew() {
    Track ntrack = trackCreateNew();
    int totalTracks = projectGetTracksNum();
    if (tracks) {
        TrackUI new = realloc(tracks, totalTracks*sizeof(struct track_control_ui));
        if (!new) return;   // We're cooked here
        tracks = new;
    } else {
        tracks = malloc(sizeof(struct track_control_ui));
        if (!tracks) return;
    }
    tracks[totalTracks-1].track = ntrack;
    customizeNewTrackUI(tracks+totalTracks-1);
    textboxLoadText(tracks[totalTracks-1].textbox, trackGetTitle(ntrack));
    sliderUpdateSlideValue(tracks[totalTracks-1].slider, trackGetVelocity(ntrack));
    globalHandlerSelectTrack(totalTracks-1);
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

    for (int i=idx; i<totalTracks-1; i++) {
        tracks[i] = tracks[i+1];
    }
    
    if (totalTracks>1) tracks = realloc(tracks, (totalTracks-1)*sizeof(struct track_control_ui));
    else {
        free(tracks);
        tracks = NULL;
    }
}

void renderTrackCLine() {
    DrawRectangleV((Vector2){0, controlLineHeight}, (Vector2){screenSize.x+2, trackCLineHeight}, COLOR_TRACK_C_LINE_BACKGROUND);

    float space = floatMax(interfaceSpace1, 0.25*trackLeftWidth); float width = trackLeftWidth-2*space;
    Rectangle rect = {space, controlLineHeight+0.15*trackCLineHeight, width, 0.7*trackCLineHeight};
    buttonUpdateRectangle(addTrackButton, rect);
    buttonUpdate(addTrackButton, -1);

    float roundness = buttonGetRoundness(addTrackButton);
    float effect = buttonGetEffectValue(addTrackButton);
    Color col1 = {23, 25, 29, 255}, col2={32, 35, 40, 255};
    Color blend1 = blendColors(col1, col2, effect);
    DrawRectangleRounded(rect, roundness, 8, blend1);

    Rectangle icRect = {space+interfaceSpace1, controlLineHeight+0.25*trackCLineHeight, 0.5*trackCLineHeight, 0.5*trackCLineHeight};
    renderFontStringAlign(GlobalFonts[0].font, "Add Track", (Vector2){0.5*(2*space+interfaceSpace1+width+icRect.width), controlLineHeight+0.5*trackCLineHeight}, (Vector2){0.5,0.5}, 0.42*trackCLineHeight, 0, COLOR_TEXT_1);
    iconRerder(T_ICON_ADD, icRect, COLOR_TEXT_1);

    
    if (isButtonClicked(addTrackButton)) renderTrackCreateNew();


    //Color topCol = {120, 139, 179, 255}, bottomCol={120, 139, 179, 0};
    float ypos = controlLineHeight+trackCLineHeight;
    //DrawRectangleGradientV(0, ypos, trackLeftWidth, 4, topCol, bottomCol);
    DrawLineEx((Vector2){0, ypos}, (Vector2){trackLeftWidth, ypos}, 2, COLOR_TEXT_4);

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
    //trackDeleteAtIdx(sel);
    renderTrackDeleteAtIdx(sel);

    int totalTracks = projectGetTracksNum();
    if (sel>=totalTracks) globalHandlerSelectTrack(totalTracks-1);
    else globalHandlerSelectTrack(sel);
}


void changeSelectedTrackColorApproach1() {
    int sel = globalHandlerGetSelectedTrack();
    if (sel<0) return;

    tracks[sel].themeIdx = (tracks[sel].themeIdx+1)%(sizeof(trackThemeColors)/sizeof(Color));
    tracks[sel].theme = trackThemeColors[tracks[sel].themeIdx];
}

void precomputeTrackOptionLayout(TrackUI track) {
    if (!track || !(track->btnList)) return;

    Rectangle trect = buttonGetRectangle(track->optionsButton);
    Rectangle brect = {trect.x+trect.width+interfaceSpace1, trect.y, buttonList4x5ExampleRect.width, buttonList4x5ExampleRect.height};
    float space = 10;
    brect = rectangleMoveToFitInsideRect(brect, (Rectangle){0, trackDivVisiblePosMin+space, screenSize.x, trackDivVisiblePosMax-trackDivVisiblePosMin-2*space});
    buttonListUpdateRect(track->btnList, brect);
    buttonListUpdateSpacing(track->btnList, buttonList4x5ExampleSpacing);
    buttonListUpdate(track->btnList);

    OnClickFunc funcs[] = {changeSelectedTrackColorApproach1, NULL, NULL, NULL, deleteSelectedTrack};
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

void precomputeTrackLeft(int idx) {
    int isSelected=(globalHandlerGetSelectedTrack()==idx), disableHover=!CheckCollisionPointRec(globalMouseHandler.pos, trackDivLeftRect);
    TrackUI track = tracks+idx;
    Track trackAbstr = trackGetAtIdx(idx);
    track->track = trackAbstr;
    
    float ypos = controlLineHeight+5+trackCLineHeight + idx*trackHeight-trackScrollY;
    Rectangle baseRect = {interfaceSpace1, ypos+interfaceSpace1*0.5, trackLeftWidth-2*interfaceSpace1, trackHeight-interfaceSpace1};
    buttonUpdateRectangle(track->base, baseRect);
    if (disableHover) buttonDisableHover(track->base);
    else buttonEnableHover(track->base);
    buttonUpdate(track->base, isSelected?1:-1);
    float targetEffect = isSelected?1:(0.5*isButtonHovered(track->base));
    track->effect += 0.15*(targetEffect-track->effect);

    if (!isSelected && isButtonClicked(track->base)) globalHandlerSelectTrack(idx);
    

    // Textbox
    Rectangle tbxRect = {baseRect.x+0.6*baseRect.height-5, baseRect.y+0.15*baseRect.height, baseRect.width-1.2*baseRect.height+10, 0.32*baseRect.height};
    if (isSelected) textboxEnable(track->textbox);
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
    if (isSelected) sliderEnable(track->slider);
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
}


void renderTrackLeft(int idx) {
    int isSelected = (globalHandlerGetSelectedTrack()==idx);
    TrackUI track = tracks+idx;
    
    Rectangle baseRect = buttonGetRectangle(track->base);
    if (baseRect.y>trackDivVisiblePosMax || baseRect.y+baseRect.height<trackDivVisiblePosMin) return;

    float ypos = controlLineHeight+5+trackCLineHeight + idx*trackHeight-trackScrollY;
    float effect = track->effect;

    Color col1={25,27,30,255}; //col2={36,40,47,255};
    Color blendBackground = blendColors(col1, track->theme, effect*0.1); //blendColors(col1, col2, effect);
    DrawRectangleRounded(baseRect, 0.2, 8, blendBackground);
    if (effect>1e-3) {
        Color tcol=track->theme; tcol.a=(unsigned char)lerp(0, 255, effect);
        float ty=ypos+0.5*trackHeight, tx=baseRect.x+baseRect.width-0.5*interfaceSpace1, tw=interfaceSpace1, th=trigInterpolation(0, trackHeight-2*interfaceSpace1, effect);
        renderRoundedRectangleCentered((Vector2){tx, ty}, (Vector2){tw, th}, tcol, 1, 8);
    }
    if (!isSelected && isButtonClicked(track->base)) globalHandlerSelectTrack(idx);


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
        iconRerder(track->icon, scaleRctangleFromCenter(icRect, 2*normalizeProgramTypeIcon(track->icon)), track->theme);
    }


    // Options button
    Rectangle opRect = buttonGetRectangle(track->optionsButton);
    float opEffect = buttonGetEffectValue(track->optionsButton);
    Color opbg = blendColors(blendBackground, (Color){45,49,57,255}, opEffect);
    if (opEffect>1e-3) DrawRectangleRounded(scaleRctangleFromCenter(opRect, lerp(0.3, 1, opEffect)), buttonGetRoundness(track->optionsButton), 8, opbg);
    iconRerder(T_ICON_OPTIONS, scaleRctangleFromCenter(opRect, 0.75*lerp(0.9, 0.95, opEffect)), blendColors(track->theme, COLOR_TEXT_1, opEffect));

    //if (track->btnList) renderTrackOptionLayout(track);

}


void renderActualTracksLeft() {
    int totalTracks = projectGetTracksNum();

    if (totalTracks){
        for (int i=0; i<totalTracks; i++) renderTrackLeft(i);
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

void order1PrecomputeTracksLeft() {
    trackCLineHeight = floatMax(controlLineHeight*0.6, 30);
    trackHeight = floatMax(85, screenSize.y*0.085);
    trackLeftWidth = floatMax(280, 0.2*screenSize.x);    
}

void order2PrecomputeTracksLeft() {
    trackDivTargetHeight = screenSize.y-controlLineHeight-trackCLineHeight-bottomHalfHeight;
    trackDivVisiblePosMin=controlLineHeight+trackCLineHeight, trackDivVisiblePosMax=screenSize.y-bottomHalfHeight;
    trackDivLeftRect = (Rectangle){0, trackDivVisiblePosMin, trackLeftWidth, trackDivTargetHeight};

    int mouseInDiv = !UIisHoveringOverLayout() && !UIisInTextInput() && CheckCollisionPointRec(globalMouseHandler.pos, trackDivLeftRect);
    int totalTracks = projectGetTracksNum();
    float totalHeight = totalTracks*trackHeight+10;
    if (mouseInDiv && totalHeight>trackDivTargetHeight && globalMouseHandler.scroll!=0 && !UIexistsFrontLayoutOverlay()) {
        trackScrollYtarget -= 0.5*trackHeight*globalMouseHandler.scroll;
    }
    if (trackScrollYtarget>totalHeight-trackDivTargetHeight) trackScrollYtarget = totalHeight-trackDivTargetHeight; // totalHeight>trackDivTargetHeight && 
    if (trackScrollYtarget<0) trackScrollYtarget=0;

    trackScrollY += 0.18*(trackScrollYtarget-trackScrollY);
    
    for (int i=0; i<totalTracks; i++) precomputeTrackLeft(i);
}



void renderTracksLeftLayoutsIfAny() {
    int selected = globalHandlerGetSelectedTrack();

    if (selected>=0 && tracks[selected].btnList) renderTrackOptionLayout(tracks+selected);
}


void renderTracksLeft() {
    

    
    renderActualTracksLeft();
    renderTrackCLine();
    
    
}






