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




int instrumentPickerExists=0, instrumentPickerSelectedType=MPT_PIANO, instrumentPickerSelectedProgram=0, instrumentPickerTrack=-1;
float pickerSize = 0.2;
Button instrumentPickerLayout = NULL;
Button instrumentPickerTypeButtons[MPT_END] = {NULL};
Button instrumentPickerProgramButtons[8] = {NULL};




void createInstrumentPicker() {
    if (instrumentPickerExists) destroyInstrumentPicker();
    
    instrumentPickerTrack=globalHandlerGetSelectedTrack();
    if (instrumentPickerTrack<0) return;

    instrumentPickerSelectedProgram = trackGetProgram(trackGetAtIdx(instrumentPickerTrack));
    if (instrumentPickerSelectedProgram>128 || instrumentPickerSelectedProgram<0) return;

    instrumentPickerSelectedType=midiGetProgramType(instrumentPickerSelectedProgram);
    instrumentPickerExists=1;
    pickerSize = 0.2;

    instrumentPickerLayout = buttonCreate((Rectangle){20,20,20,20}, 0.1);
    buttonEnableOnFrontLayout(instrumentPickerLayout);
    buttonUpdateCursorOnHover(instrumentPickerLayout, MOUSE_CURSOR_DEFAULT);

    for (int i=0; i<MPT_END; i++) {
        instrumentPickerTypeButtons[i] = buttonCreate((Rectangle){20,20,20,20}, 0.2);
        buttonEnableOnFrontLayout(instrumentPickerTypeButtons[i]);
    }

    //struct midi_programs_array availableProgs = midiGetProgramsByType(instrumentPickerSelectedType);
    for (int i=0; i<8 /*availableProgs.num*/; i++) {
        instrumentPickerProgramButtons[i] = buttonCreate((Rectangle){20,20,20,20}, 0.2);
        buttonEnableOnFrontLayout(instrumentPickerProgramButtons[i]);
    }

    UIcreateFrontLayoutOverlay();
}



void destroyInstrumentPicker() {
    instrumentPickerExists=0;
    instrumentPickerSelectedType=MPT_PIANO;
    instrumentPickerSelectedProgram=0;
    instrumentPickerTrack=-1;
    pickerSize = 0.2;

    if (instrumentPickerLayout) buttonFree(instrumentPickerLayout);

    for (int i=0; i<MPT_END; i++) {
        if (instrumentPickerTypeButtons[i]) buttonFree(instrumentPickerTypeButtons[i]);
        instrumentPickerTypeButtons[i] = NULL;
    }

    for (int i=0; i<8; i++) {
        if (instrumentPickerProgramButtons[i]) buttonFree(instrumentPickerProgramButtons[i]);
        instrumentPickerProgramButtons[i] = NULL;
    }

    UIdestroyFrontLayoutOverlay();
}


void selectInstrumentAction() {
    if (!instrumentPickerTypeButtons[0] || !instrumentPickerProgramButtons[0]) return;
    int trackIdx = globalHandlerGetSelectedTrack();
    if (trackIdx<0) return;

    Track track = trackGetAtIdx(trackIdx);

    struct midi_programs_array availableProgs = midiGetProgramsByType(instrumentPickerSelectedType);
    for (int i=0; i<availableProgs.num; i++) {
        if (isButtonClicked(instrumentPickerProgramButtons[i])) {
            int selProg = midiProgramGetProgramNum(availableProgs.array[i]);
            trackSetProgram(track, (uint8_t)selProg);
            instrumentPickerSelectedProgram = selProg;
        }
    }
    midiFreeMidiProgramArray(&availableProgs);
}


void precalculateInstrumentPicker() {
    if (!instrumentPickerExists || !instrumentPickerLayout) return;
    pickerSize += 0.18*(1-pickerSize);

    float oh=floatMax(500, 0.5*screenSize.y), ow=floatMax(600, 0.4*screenSize.x);
    float height=oh*pickerSize;
    float width=ow*pickerSize;

    Rectangle rect = centerRectangle(Vector2Scale(screenSize, 0.5), (Vector2){width, height});
    buttonUpdateRectangle(instrumentPickerLayout, rect);
    buttonUpdate(instrumentPickerLayout, -1);

    if (globalMouseHandler.pressed && !isButtonClicked(instrumentPickerLayout)) actionDefer(destroyInstrumentPicker);


    float buttonsLeftSize = pickerSize*(oh-2*interfaceSpace2-8*interfaceSpace1)/9;
    float x1=rect.x+interfaceSpace2*pickerSize; float x2=x1+buttonsLeftSize+pickerSize*interfaceSpace1;
    float curY=rect.y+interfaceSpace2*pickerSize;
    for (int i=MPT_PIANO; i<MPT_END-1; i+=2) {
        Rectangle trect = {x1, curY, buttonsLeftSize, buttonsLeftSize};
        buttonUpdateRectangle(instrumentPickerTypeButtons[i], trect);
        buttonUpdate(instrumentPickerTypeButtons[i], (instrumentPickerSelectedType==i)?1:-1);

        if (isButtonClicked(instrumentPickerTypeButtons[i])) instrumentPickerSelectedType=i;

        trect.x=x2;
        buttonUpdateRectangle(instrumentPickerTypeButtons[i+1], trect);
        buttonUpdate(instrumentPickerTypeButtons[i+1], (instrumentPickerSelectedType==i+1)?1:-1);
        if (isButtonClicked(instrumentPickerTypeButtons[i+1])) instrumentPickerSelectedType=i+1;
        curY+=buttonsLeftSize+interfaceSpace1*pickerSize;
    }

    Rectangle trect = {x1, curY, buttonsLeftSize*2+interfaceSpace1*pickerSize, buttonsLeftSize};
    buttonUpdateRectangle(instrumentPickerTypeButtons[MPT_END-1], trect);
    buttonUpdate(instrumentPickerTypeButtons[MPT_END-1], (instrumentPickerSelectedType==MPT_END-1)?1:-1);
    if (isButtonClicked(instrumentPickerTypeButtons[MPT_END-1])) instrumentPickerSelectedType=MPT_END-1;


    float buttonRightHeight = pickerSize*(oh-7*interfaceSpace1-2*interfaceSpace2)/8, buttonRightX=x2+buttonsLeftSize+2*interfaceSpace2*pickerSize; float buttonRightWidth=rect.x+width-interfaceSpace2*pickerSize-buttonRightX;
    curY=rect.y+interfaceSpace2*pickerSize;
    struct midi_programs_array availableProgs = midiGetProgramsByType(instrumentPickerSelectedType);
    for (int i=0; i<availableProgs.num; i++) {
        Rectangle trect = {buttonRightX, curY, buttonRightWidth, buttonRightHeight};
        buttonUpdateRectangle(instrumentPickerProgramButtons[i], trect);
        buttonUpdate(instrumentPickerProgramButtons[i], (instrumentPickerSelectedProgram==midiProgramGetProgramNum(availableProgs.array[i]))?1:-1);
        if (isButtonClicked(instrumentPickerProgramButtons[i])) actionDefer(selectInstrumentAction);

        curY+=buttonRightHeight+interfaceSpace1*pickerSize;
    }
    midiFreeMidiProgramArray(&availableProgs);
    //for (int i=availableProgs.num; i<8; i++) {}
}



void renderInstrumentPicker() {
    if (!instrumentPickerExists || !instrumentPickerLayout) return;

    Rectangle rect = buttonGetRectangle(instrumentPickerLayout);
    float roundness = buttonGetRoundness(instrumentPickerLayout);
    Color col = {20, 20, 20, 255};
    

    Track track = trackGetAtIdx(globalHandlerGetSelectedTrack());
    int programIdx = trackGetProgram(track);
    enum midi_program_type mt = midiGetProgramType(programIdx);
    Color typeTheme = programTypeColors[mt];
    Color outlineCol = blendColors((Color){200, 200, 200, 255}, typeTheme, 0.35);
    DrawRectangleRounded(rect, roundness, 8, blendColors(col, typeTheme, 0.00));
    DrawRectangleRoundedLinesEx(rect, roundness, 8, 3, outlineCol);
    Rectangle tr = buttonGetRectangle(instrumentPickerTypeButtons[1]);
    float tx = tr.x+tr.width+pickerSize*interfaceSpace2;
    DrawLineEx((Vector2){tx, rect.y}, (Vector2){tx, rect.y+rect.height}, 3, outlineCol);


    for (int i=MPT_PIANO; i<MPT_END; i++) {
        Rectangle trect = buttonGetRectangle(instrumentPickerTypeButtons[i]);
        float roundness = buttonGetRoundness(instrumentPickerTypeButtons[i]);
        float effect = buttonGetEffectValue(instrumentPickerTypeButtons[i]);

        DrawRectangleRounded(trect, roundness, 4, blendColors((Color){30, 30, 30, 255}, programTypeColors[i], lerp(0, 0.5, effect)));
        enum icon_title icon = midiGetTypeIconFromType(i);
        iconRerder(icon, scaleRctangleFromCenter(rectangleScaleToFitInCenter((Vector2){1,1}, trect), normalizeProgramTypeIcon(icon)), blendColors(programTypeColors[i], (Color){220, 220, 220, 255}, lerp(0.3, 1, effect)));
    }


    struct midi_programs_array availableProgs = midiGetProgramsByType(instrumentPickerSelectedType);
    for (int i=0; i<availableProgs.num; i++) {
        MidiProgram prog = availableProgs.array[i];
        Rectangle trect = buttonGetRectangle(instrumentPickerProgramButtons[i]);
        float roundness = buttonGetRoundness(instrumentPickerProgramButtons[i]);
        float effect = buttonGetEffectValue(instrumentPickerProgramButtons[i]);
        Color bgc = blendColors((Color){30, 30, 30, 255}, programTypeColors[instrumentPickerSelectedType], lerp(0.1, 0.5, effect));
        Color fgc = blendColors(programTypeColors[instrumentPickerSelectedType], (Color){220, 220, 220, 255}, lerp(0.5, 1, effect));

        DrawRectangleRounded(trect, roundness, 6, bgc);
        renderFontStringAlign(GlobalFonts[0].font, midiProgramGetName(prog), (Vector2){trect.x+pickerSize*interfaceSpace2, trect.y+0.5*trect.height}, (Vector2){0,0.5}, 0.5*trect.height, 0, fgc);
    }
    midiFreeMidiProgramArray(&availableProgs);
}