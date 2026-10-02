#include <raylib.h>
#include <utils.h>
#include <interface.h>
#include <colors.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <export.h>
#include "tiles.h"
#include <export.h>
#include <string.h>
#include <waterfall.h>





#define PROJECT_TITLE_PLACEHOLDER "Project Title"




static Rectangle targetRect={0,0,0,0};
static RenderTexture2D expTexture={0,};

static Color tilesBackgroundColor={0,0,0,0}, disabledBackgroundColor={0,0,0,0}, textOrHoverBackgroundColor={0,0,0,0}, settingsBackgroundColor={0,0,0,0};
static int expVideoActive=0, beatsInMeasure=4;
static double curTime=0.0;


void tilesSetUpTargetRectangle(Rectangle rect) {
    targetRect = rect;
}


void tilesSetUpColors(Color tilesBackground, Color disabledBackground, Color textOrHoverBackground, Color settingsBackground) {
    tilesBackgroundColor=tilesBackground;
    disabledBackgroundColor=disabledBackground;
    textOrHoverBackgroundColor=textOrHoverBackground;
    settingsBackgroundColor=settingsBackground;
}





void tilesExportVideoClose() {
    if (expTexture.id) UnloadRenderTexture(expTexture);
    expTexture=(RenderTexture2D){0,};
    expVideoActive = 0;
}


void tilesExportVideoInit() {
    if (expTexture.id) UnloadRenderTexture(expTexture);

    int width=0, height=0;
    tilesSettingsGetResolution(&width, &height);
    expTexture=LoadRenderTexture(width, height);
    if (!(expTexture.id)) {
        tilesExportVideoClose();
        return;
    }
    expVideoActive = 1;
    tilesSetUpTargetRectangle((Rectangle){0,0,width,height});
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


static void _renderPianoKeysClassic() {
    float heighKeyPerc=view.keyHeight, blackKeyHeight=view.blackKeyHeight, blackKeyWidth=view.blackKeyWidth;
    float y=targetRect.y+targetRect.height*(1.0-heighKeyPerc)-view.spacePerc*targetRect.width, w=view.whiteKeyPerc*targetRect.width, h=heighKeyPerc*targetRect.height, gap=floatMin(w*0.65, h*0.2);
    Color cols[] = {(Color){190,190,190,255}, (Color){25,25,25,255}, (Color){130,130,130,255}, (Color){12,12,12,255}};

    uint8_t keyType[] = {0,1,0,1,0,0,1,0,1,0,1,0};
    for (int iteration=0; iteration<2; iteration++) {
        float x=targetRect.x+view.spacePerc*targetRect.width+iteration*(w-0.5*blackKeyWidth*w);
        for (int i=settings.info.minShownKey; i<=settings.info.maxShownKey; i++) {
            int mod = i%12;
            uint8_t type = keyType[mod];
            if (type!=iteration) {
                if (mod==11 || mod==4) x+=w;
                continue;
            }

            Rectangle rect;
            Color col = cols[type+2];
            float force = (view.columns && view.columns[i-settings.info.minShownKey].isDown)?floatClip(view.columns[i-settings.info.minShownKey].holdForce,0.0,1.0):0.0;
            if (iteration==0) rect = (Rectangle){x+1,y,w-2,h};
            else rect = (Rectangle){x,y,w*blackKeyWidth,h*blackKeyHeight};
            if (force>0) col = blendColors(col, view.columns[i-settings.info.minShownKey].color, 0.3*force);

            DrawRectangleRec(rect, col);
            col = blendColors(col, BLACK, 0.4+0.4*type);
            DrawRectangleRec((Rectangle){x+1,y+h*(type?blackKeyHeight:1.0)-gap*(0.3+0.7*(1.0-force)),w*(type?blackKeyWidth:1.0)-2,gap*0.15}, col);


            col = cols[type];
            if (iteration==0) rect = (Rectangle){x+1,y,w-2,h-gap*(0.3+0.7*(1.0-force))};
            else rect = (Rectangle){x,y,w*blackKeyWidth,h*blackKeyHeight-gap*(0.3+0.7*(1.0-force))};
            if (force>0) col = blendColors(col, view.columns[i-settings.info.minShownKey].color, 0.85*force);

            DrawRectangleRec(rect, col);

            x += w;
        }
    }

    int gradient=0;
    if (gradient) {
        Color grcol[] = {(Color){160,63,156,60}, (Color){116,50,110,0}};
        float x=targetRect.x+view.spacePerc*targetRect.width, d=floatMax(0.5*w, 0.02*targetRect.height);
        DrawRectangleGradientEx((Rectangle){x,y-d,w*view.whiteKeys,d}, grcol[1], grcol[0], grcol[0], grcol[1]);
    }
}

static void _renderPianoKeysProject() {
    
    float heighKeyPerc=view.keyHeight, blackKeyHeight=view.blackKeyHeight, blackKeyWidth=view.blackKeyWidth;
    float y=targetRect.y+targetRect.height*(1.0-heighKeyPerc)-view.spacePerc*targetRect.width, w=view.whiteKeyPerc*targetRect.width, h=heighKeyPerc*targetRect.height;
    float space=0.1*w, pixRad=0.12*w, gradH=0.25*h;
    Color cols[] = {blendColors(COLOR_KEYBOARD_H_KEY_SELECTED_WHITE, WHITE, 0.2), blendColors(COLOR_KEYBOARD_H_KEY_SELECTED_BLACK, BLACK, 0.2), COLOR_KEYBOARD_H_KEY_SELECTED_WHITE_HOVERED, COLOR_KEYBOARD_H_KEY_SELECTED_BLACK_HOVERED};
    

    uint8_t keyType[] = {0,1,0,1,0,0,1,0,1,0,1,0};
    uint8_t rendType[] = {0,3,2,3,1,0,3,2,3,2,3,1};
    for (int iteration=0; iteration<2; iteration++) {
        float x=targetRect.x+view.spacePerc*targetRect.width+iteration*(w-0.5*blackKeyWidth*w);
        for (int i=settings.info.minShownKey; i<=settings.info.maxShownKey; i++) {
            int mod = i%12;
            uint8_t type = keyType[mod];
            if (type!=iteration) {
                if (mod==11 || mod==4) x+=w;
                continue;
            }



            int down = (view.columns && view.columns[i-settings.info.minShownKey].isDown);
            float force = down?floatClip(view.columns[i-settings.info.minShownKey].holdForce,0.0,1.0):0.0;
            Color col = down?blendColors(cols[type], view.columns[i-settings.info.minShownKey].color, force):cols[type];   // cols[type+2*down]


            if (iteration==0) {
                int rtype = rendType[mod];
                

                Rectangle rect={x+0.5*space,y,w-space,h};
                if (rtype==0 || rtype==1) {
                    rect.height = 2*pixRad;
                    DrawRectangleRounded(rect, getRoundnessForRoundedRectangle(rect, pixRad), 4, col);
                }
                if (rtype==0) rect = (Rectangle){rect.x,y+pixRad,w-space-pixRad,h*blackKeyHeight+space};
                else if (rtype==2) rect = (Rectangle){x+0.5*blackKeyWidth*w+space-pixRad,y,w-blackKeyWidth*w-space+2*pixRad,h*blackKeyHeight+space};
                else rect = (Rectangle){x+0.5*blackKeyWidth*w+space-pixRad,y+pixRad,w+pixRad-1.5*space-0.5*blackKeyWidth*w,h*blackKeyHeight+space};
                DrawRectangleRec(rect, col);

                rect = (Rectangle){x+0.5*space,y+h*blackKeyHeight+space,w-space,h-(h*blackKeyHeight+space)};
                DrawRectangleRounded(rect, getRoundnessForRoundedRectangle(rect, pixRad), 4, col);

                if (0) {
                    rect = (Rectangle){x,y-h*0.3,w,h*0.3};
                    Color c1=blendColors(col, BLACK, 0.25);
                    Color c2=c1; c1.a=(unsigned char)(120*force), c2.a=0;
                    DrawRectangleGradientEx(rect, c2, c1, c1, c2);
                }
                
            } else {
                Rectangle rect = {x-space,y,w*blackKeyWidth+2*space,h*blackKeyHeight+space-pixRad};
                DrawRectangleRec(rect, COLOR_BACKGROUND_1);
                rect.y+=rect.height-pixRad;
                rect.height = 2*pixRad;
                DrawRectangleRounded(rect, 1.0, 4, COLOR_BACKGROUND_1);

                rect = (Rectangle){x,y,w*blackKeyWidth,h*blackKeyHeight-pixRad};
                DrawRectangleRec(rect, col);
                rect.y+=rect.height-pixRad;
                rect.height = 2*pixRad;
                DrawRectangleRounded(rect, 1.0, 4, col);

                if (0) {
                    rect = (Rectangle){x,y-h*0.3,w*blackKeyWidth,h*0.3};
                    Color c1=blendColors(col, WHITE, 0.15);
                    Color c2=c1; c1.a=(unsigned char)(120*force), c2.a=0;
                    DrawRectangleGradientEx(rect, c2, c1, c1, c2);
                }
            }

            if (down) {
                Rectangle rect = {targetRect.x+targetRect.width*view.columns[i-settings.info.minShownKey].x-2,y-gradH,targetRect.width*view.columns[i-settings.info.minShownKey].w+4,gradH};
                Color c1=blendColors(col, WHITE, 0.5);
                Color c2=c1; c1.a=(unsigned char)(120*force), c2.a=0;
                DrawRectangleGradientEx(rect, c2, c1, c1, c2);
            }
            x += w;
        }
    }
}


static void _renderPianoKeysClean() {
    float heighKeyPerc=view.keyHeight, blackKeyHeight=view.blackKeyHeight, blackKeyWidth=view.blackKeyWidth;
    float y=targetRect.y+targetRect.height*(1.0-heighKeyPerc)-view.spacePerc*targetRect.width, w=view.whiteKeyPerc*targetRect.width, h=heighKeyPerc*targetRect.height, gradH=0.25*h;
    Color cols[] = {(Color){180,180,180,255}, (Color){25,25,25,255}};

    uint8_t keyType[] = {0,1,0,1,0,0,1,0,1,0,1,0};
    for (int iteration=0; iteration<2; iteration++) {
        float x=targetRect.x+view.spacePerc*targetRect.width+iteration*(w-0.5*blackKeyWidth*w);
        for (int i=settings.info.minShownKey; i<=settings.info.maxShownKey; i++) {
            int mod = i%12;
            uint8_t type = keyType[mod];
            if (type!=iteration) {
                if (mod==11 || mod==4) x+=w;
                continue;
            }

            Rectangle rect;
            Color col = cols[type];
            int down=(view.columns && view.columns[i-settings.info.minShownKey].isDown);
            float force = down?floatClip(view.columns[i-settings.info.minShownKey].holdForce,0.0,1.0):0.0;
            if (iteration==0) rect = (Rectangle){x+1,y,w-2,h};
            else rect = (Rectangle){x,y,w*blackKeyWidth,h*blackKeyHeight};
            if (down) col = blendColors(col, view.columns[i-settings.info.minShownKey].color, (iteration?0.7:0.9)*force);

            DrawRectangleRec(rect, col);


            if (down) {
                rect = (Rectangle){targetRect.x+targetRect.width*view.columns[i-settings.info.minShownKey].x,y-gradH,targetRect.width*view.columns[i-settings.info.minShownKey].w,gradH};// (Rectangle){x,y-h*0.3,w*blackKeyWidth,h*0.3};
                Color c1=blendColors(col, WHITE, 0.15);
                Color c2=c1; c1.a=(unsigned char)(120*force), c2.a=0;
                DrawRectangleGradientEx(rect, c2, c1, c1, c2);
            }
            x += w;
        }
    }

    int gradient=0;
    if (gradient) {
        Color grcol[] = {(Color){160,63,156,60}, (Color){116,50,110,0}};
        float x=targetRect.x+view.spacePerc*targetRect.width, d=floatMax(0.5*w, 0.02*targetRect.height);
        DrawRectangleGradientEx((Rectangle){x,y-d,w*view.whiteKeys,d}, grcol[1], grcol[0], grcol[0], grcol[1]);
    }
}


static void _renderRollBackgroundStripes() {
    float heighKeyPerc=view.keyHeight, blackKeyWidth=0.5;//view.blackKeyWidth;
    float y=targetRect.y, w=view.whiteKeyPerc*targetRect.width, h=targetRect.height*(1.0-heighKeyPerc)-view.spacePerc*targetRect.width;
    float w1=w-0.5*w*blackKeyWidth, w2=w*blackKeyWidth, w3=w*(1.0-blackKeyWidth);
    float offsets[] = {w1,w2,w3,w2,w1,w1,w2,w3,w2,w3,w2,w1};
    uint8_t keyType[] = {0,1,0,1,0,0,1,0,1,0,1,0};
    Color cols[] = {(Color){35,39,43,255}, (Color){24,29,32,255}, (Color){65,69,75,255}, (Color){35,37,41,255}};
    for (int i=0; i<2; i++) cols[i] = blendColors(cols[i], (Color){0,0,0,255}, 0.3);

    float x=targetRect.x+view.spacePerc*targetRect.width;
    for (int i=settings.info.minShownKey; i<=settings.info.maxShownKey; i++) {
        int mod = i%12;
        uint8_t type = keyType[mod];
        
        Rectangle rect = {x,y,offsets[mod],h};
        DrawRectangleRec(rect, cols[type]);
        if (!mod && i>settings.info.minShownKey) DrawLineV((Vector2){x,y}, (Vector2){x,y+h}, cols[2]);
        if (mod==5 && i>settings.info.minShownKey) DrawLineV((Vector2){x,y}, (Vector2){x,y+h}, cols[3]);
        x += offsets[mod];
    }
}

static void _renderRollBackgroundSingle() {
    float heighKeyPerc=view.keyHeight, blackKeyWidth=0.5;
    float y=targetRect.y, w=view.whiteKeyPerc*targetRect.width, h=targetRect.height*(1.0-heighKeyPerc)-view.spacePerc*targetRect.width;
    float w1=w-0.5*w*blackKeyWidth, w2=w*blackKeyWidth, w3=w*(1.0-blackKeyWidth);
    float offsets[] = {w1,w2,w3,w2,w1,w1,w2,w3,w2,w3,w2,w1};
    Color cols[] = {(Color){35,39,43,255}, (Color){24,29,32,255}, (Color){75,80,88,255}};
    for (int i=0; i<2; i++) cols[i] = blendColors(cols[i], (Color){0,0,0,255}, 0.3);

    float x=targetRect.x+view.spacePerc*targetRect.width;
    DrawRectangleRec((Rectangle){x,y,targetRect.width*(1.0-2*view.spacePerc), h}, cols[1]);
    for (int i=settings.info.minShownKey; i<=settings.info.maxShownKey; i++) {
        int mod = i%12;
        
        if (!mod && i>settings.info.minShownKey) DrawLineV((Vector2){x,y}, (Vector2){x,y+h}, cols[2]);
        x += offsets[mod];
    }
}

static void _renderRollBackgroundBeatBreaks() {
    int iterations=3+(settings.visibleDuration.seconds/beatDuration);
    int start = floor((curTime-settings.startDelay.seconds)/beatDuration);

    float topY=targetRect.y, x1=targetRect.x+view.spacePerc*targetRect.width, x2=targetRect.x+(1.0-view.spacePerc)*targetRect.width, bottomY=targetRect.y+targetRect.height*(1.0-view.keyHeight)-view.spacePerc*targetRect.width;
    float range = bottomY-topY;
    float textSize = floatMin(floatMin(range*beatDuration/settings.visibleDuration.seconds, targetRect.width*0.05), 0.85*targetRect.width*view.whiteKeyPerc);
    int lastBeat = beatsInMeasure*((int)ceil((settings.info.duration)/(beatDuration*beatsInMeasure)));

    for (int i=0; i<iterations; i++) {
        int itr = i+start;
        if (itr<0) continue;

        float y=bottomY-range*(itr*beatDuration-curTime+settings.startDelay.seconds)/settings.visibleDuration.seconds;
        
        if (y<topY-1) break;

        int isMeasure = (itr%beatsInMeasure==0);
        if (itr>lastBeat) break;

        if (y<=bottomY+1) DrawLineEx((Vector2){x1,y}, (Vector2){x2,y}, 1.0+1.0*isMeasure, isMeasure?COLOR_TEXT_5:COLOR_TEXT_6);
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
        case THEME_PROJECT: {
            _renderPianoKeysProject();
            return;
        }
        case THEME_CLASSIC: {
            _renderPianoKeysClassic();
            return;
        }

        case THEME_CLEAN: {
            _renderPianoKeysClean();
            return;
        }
        default: break;
    }
}

static void _renderRollBackground() {
    DrawRectangleRec(targetRect, COLOR_BACKGROUND_1);

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

typedef void (*RenderNoteFn)(Note note, float y1, float y2, Color col);
typedef Color (*ColorNoteFn)(int type, int track);
typedef void (*RenderTitleFn)(const char* title);



static void _renderNoteProject(Note note, float y1, float y2, Color col) {
    int idx = note->key-settings.info.minShownKey;
    int curPlayed = (note->ftimestamp-curTime+settings.startDelay.seconds<=0);
    float pixRad = targetRect.width*view.whiteKeyPerc*0.2;
    Rectangle rect = {targetRect.x+targetRect.width*view.columns[idx].x+1, y2, targetRect.width*view.columns[idx].w-2, y1-y2};
    if (curPlayed) {
        float px=3.0;//floatMax(5.0, 0.1*rect.width);
        Rectangle nrect = rectangleIncrease(rect, (Vector2){px,px}, (Vector2){px,px});
        Color ncol=col; ncol.a=80;
        DrawRectangleRounded(nrect, getRoundnessForRoundedRectangle(nrect, pixRad+px), 4, ncol);
        col = blendColors(col, WHITE, 0.15);
    }
    DrawRectangleRounded(rect, getRoundnessForRoundedRectangle(rect, pixRad), 4, col);
}

void _renderNoteClassic(Note note, float y1, float y2, Color col) {
    int idx = note->key-settings.info.minShownKey;
    int curPlayed = (note->ftimestamp-curTime+settings.startDelay.seconds<=0);
    float pixRad = targetRect.width*view.whiteKeyPerc*0.2;
    Rectangle rect = {targetRect.x+targetRect.width*view.columns[idx].x+1, y2, targetRect.width*view.columns[idx].w-2, y1-y2};
    DrawRectangleRounded(rect, getRoundnessForRoundedRectangle(rect, pixRad), 4, col);
    if (curPlayed) {
        col = blendColors(col, WHITE, 0.2);
        DrawRectangleRoundedLinesEx(rect, getRoundnessForRoundedRectangle(rect, pixRad), 4, 2.0, col);
    }
}

static void _renderNoteClean(Note note, float y1, float y2, Color col) {
    int idx = note->key-settings.info.minShownKey;
    int curPlayed = (note->ftimestamp-curTime+settings.startDelay.seconds<=0);
    float pixRad = targetRect.width*view.whiteKeyPerc*0.2;
    Rectangle rect = {targetRect.x+targetRect.width*view.columns[idx].x+1, y2, targetRect.width*view.columns[idx].w-2, y1-y2};
    if (curPlayed) col = blendColors(col, WHITE, 0.15);
    Color interior = col; interior.a=65;
    float roundness = getRoundnessForRoundedRectangle(rect, pixRad);
    DrawRectangleRounded(rect, roundness, 4, interior);
    DrawRectangleRoundedLinesEx(rect, roundness, 4, 2.0, col);
}


static Color _getNoteColorProject(int type, int track) {
    if (type) return blendColors(settings.tracks[track].color, (Color){0,0,0,255}, 0.35);
    return settings.tracks[track].color;
    //if (type) return getAnyTrackThemeColorForBlackKeys(track);
    //else return getAnyTrackThemeColorForWhiteKeys(track);
}

static Color _getNoteColorClean(int type, int track) {
    (void)type;
    return settings.tracks[track].color;    //getAnyTrackThemeColorForWhiteKeys(track);
}

static void _renderNotesInRoll() {
    RenderNoteFn renderFunctions[THEME_END] = {_renderNoteProject, _renderNoteProject, _renderNoteClean, NULL};
    ColorNoteFn colorFunctions[THEME_END] = {_getNoteColorClean, _getNoteColorProject, _getNoteColorClean, _getNoteColorProject};

    RenderNoteFn targetRender = renderFunctions[settings.theme];
    ColorNoteFn targetColor = colorFunctions[settings.theme];
    if (!targetRender || !targetColor) return;
    

    float topY=targetRect.y, bottomY=targetRect.y+targetRect.height*(1.0-view.keyHeight)-view.spacePerc*targetRect.width;
    float range = bottomY-topY;

    uint64_t n=settings.notes.size;
    Note* arr = settings.notes.notes;
    if (!arr || !n || !view.columns) return;

    uint8_t types[]={0,1,0,1,0,0,1,0,1,0,1,0};

    if (view.columns) for (int i=0; i<view.totalKeys; i++) view.columns[i].isDown=0, view.columns[i].holdForce=0, view.columns[i].color=BLACK;

    for (uint64_t i=0; i<n; i++) {
        Note nt = arr[i];

        float y1=bottomY-range*(nt->ftimestamp-curTime+settings.startDelay.seconds)/settings.visibleDuration.seconds;
        float y2=bottomY-range*(nt->ftimestamp+nt->fduration-curTime+settings.startDelay.seconds)/settings.visibleDuration.seconds;
        if (y1<topY-1) break;
        if (y2>bottomY+1 || !(settings.tracks[nt->track].show)) continue;

        int key = nt->key;
        int type = types[key%12];
        Color col = targetColor(type, nt->track);
        if (y1>bottomY && y2<bottomY) {
            struct note_column* clmn = view.columns+(key-settings.info.minShownKey);

            clmn->isDown = 1;
            float ntForce = (curTime-settings.startDelay.seconds-nt->ftimestamp)/(nt->fduration);
            ntForce = nt->velocity/127.0*(1.0-ntForce*ntForce);
            clmn->holdForce += ntForce;
            clmn->color = blendColors(clmn->color, col, ntForce/clmn->holdForce);            
        }
        targetRender(nt, y1, y2, col);
    }
}

static void _renderTitleProject(const char* title) {
    if (!title) return;
    float h = (0.1+0.02*targetRect.width/targetRect.height*(targetRect.height<targetRect.width))*targetRect.height;
    float d=0.2*h;

    Rectangle rect = {targetRect.x+0.5*d, targetRect.y+0.5*d, targetRect.width-d, h-0.5*d};
    DrawRectangleRounded(rect, getRoundnessForRoundedRectangle(rect, d*1.5), 8, (Color){15,19,24,120});

    rect = (Rectangle){targetRect.x+d, targetRect.y+d, targetRect.width-2*d, h-1.5*d};
    DrawRectangleRounded(rect, getRoundnessForRoundedRectangle(rect, d), 8, blendColors(textOrHoverBackgroundColor, COLOR_BACKGROUND_1, 0.1));

    float textSize = 0.06*rect.width;
    renderFontStringAlign(GlobalFonts[0].font, title, getRectangleCenter(rect), (Vector2){0.5,0.5}, _getTextSizeToFitInRect(rect, textSize, title, rect.height*0.5), 0, blendColors(COLOR_PALETTE_1_P9, COLOR_TEXT_1, 0.3));
}

static void _renderTitleClean(const char* title) {
    if (!title) return;
    float h = (0.1+0.02*targetRect.width/targetRect.height*(targetRect.height<targetRect.width))*targetRect.height;
    float d=0.2*h;

    Rectangle rect = {targetRect.x+d, targetRect.y+d, targetRect.width-2*d, h-1.5*d};
    Color col=COLOR_BACKGROUND_1; col.a=175;
    DrawRectangleRounded(rect, getRoundnessForRoundedRectangle(rect, d), 8, col);

    col = blendColors(COLOR_PALETTE_1_P6, COLOR_TEXT_4, 0.5);
    DrawRectangleRoundedLinesEx(rect, getRoundnessForRoundedRectangle(rect, d), 8, d*0.15, col);

    col = blendColors(COLOR_PALETTE_1_P6, COLOR_TEXT_3, 0.5);
    float textSize = 0.06*rect.width;
    renderFontStringAlign(GlobalFonts[0].font, title, getRectangleCenter(rect), (Vector2){0.5,0.5}, _getTextSizeToFitInRect(rect, textSize, title, rect.height*0.5), 0, col);
}

static void _renderTitleClassic(const char* title) {
    if (!title) return;
    float h = (0.1+0.02*targetRect.width/targetRect.height*(targetRect.height<targetRect.width))*targetRect.height;
    float d=0.2*h;
    Rectangle rect = {targetRect.x+0.5*d, targetRect.y+0.5*d, targetRect.width-d, h-0.5*d};
    DrawRectangleRounded(rect, getRoundnessForRoundedRectangle(rect, d*1.5), 8, (Color){15,19,24,120});
    rect = (Rectangle){targetRect.x+d, targetRect.y+d, targetRect.width-2*d, h-1.5*d};
    DrawRectangleRounded(rect, getRoundnessForRoundedRectangle(rect, d), 8, blendColors(textOrHoverBackgroundColor, COLOR_BACKGROUND_1, 0.15));
    renderFontStringAlign(GlobalFonts[0].font, title, getRectangleCenter(rect), (Vector2){0.5,0.5}, _getTextSizeToFitInRect(rect, 0.06*rect.width, title, rect.height*0.5), 0, COLOR_TEXT_1);
}

static void _renderTitleTest1(const char* title) {
    if (!title) return;
    float h = 0.1*targetRect.height;
    Rectangle rect = {targetRect.x, targetRect.y, targetRect.width, h};
    Color col = (Color){24,26,29,255};
    DrawRectangleRec(rect, col);

    float textSize = 0.06*rect.width;
    float finalSize = _getTextSizeToFitInRect(rect, textSize, title, rect.height*0.5);

    renderFontStringAlign(GlobalFonts[0].font, title, getRectangleCenter(rect), (Vector2){0.5,0.5}, finalSize, 0, COLOR_PALETTE_1_P9);
    rect.y += rect.height; rect.height = 2;
    DrawRectangleRec(rect, blendColors(COLOR_PALETTE_1_P9, (Color){210,210,210,255}, 0.4));
    rect.y += rect.height; rect.height = 0.2*h;
    Color top=col, bot=col;
    top.a=80, bot.a=0;
    DrawRectangleGradientEx(rect, top, bot, bot, top);
}


static void _renderTitle() {
    RenderTitleFn renderFunctions[THEME_END] = {_renderTitleProject, _renderTitleClassic, _renderTitleClean, _renderTitleTest1};
    RenderTitleFn target = renderFunctions[settings.theme];

    if (!target) return;

    const char* title = projectGetCurrentTitle();
    if (!title || strlen(title)==0) title = PROJECT_TITLE_PLACEHOLDER;

    target(title);

}

static void _renderKeyboardBackground() {
    float y=targetRect.y+targetRect.height*(1.0-view.keyHeight)-view.spacePerc*targetRect.width, h=view.keyHeight*targetRect.height+view.spacePerc*targetRect.width;
    DrawRectangleRec((Rectangle){targetRect.x,y,targetRect.width,h}, COLOR_BACKGROUND_1);
}

static void _renderPreviewImperfectionOverlay() {
    DrawRectangleRec((Rectangle){previewDiv.x,0,previewDiv.width,previewDiv.y}, COLOR_BACKGROUND_1);

    if (targetRect.y>previewDiv.y) {
        DrawRectangleRec((Rectangle){targetRect.x,previewDiv.y,targetRect.width,targetRect.y-previewDiv.y}, tilesBackgroundColor);
        DrawRectangleRec((Rectangle){targetRect.x,targetRect.y+targetRect.height,targetRect.width,screenSize.y-(targetRect.y+targetRect.height)}, tilesBackgroundColor);
    }

    if (targetRect.x>previewDiv.x+divsPixelRadius) {
        DrawRectangleRec((Rectangle){previewDiv.x+divsPixelRadius,previewDiv.y,targetRect.x-previewDiv.x-divsPixelRadius,previewDiv.height}, tilesBackgroundColor);
        DrawRectangleRec((Rectangle){targetRect.x+targetRect.width,previewDiv.y,previewDiv.x+previewDiv.width-(targetRect.x+targetRect.width),previewDiv.height}, tilesBackgroundColor);
    }
}



void _renderTilesToTargetRect() {
    if (expVideoActive) curTime = exportVideoSimulationGetTime();
    else curTime = _tilesControlGetTime();

    beatsInMeasure = settings.beatsInMeasure;

    _renderRollBackground();
    _renderNotesInRoll();
    _renderKeyboardBackground();
    if (!expVideoActive) _renderPreviewImperfectionOverlay();
    _renderPianoRoll();
    _renderTitle();
}



void tilesVideoExportRenderFrame() {
    if (!expTexture.id) return;

    BeginTextureMode(expTexture);
        _renderTilesToTargetRect();
    EndTextureMode();
}

Image tilesGetImage() {
    if (!expTexture.id) return (Image){NULL,};
    return LoadImageFromTexture(expTexture.texture);
}