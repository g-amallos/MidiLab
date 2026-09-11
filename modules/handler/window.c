#include <raylib.h>
#include <string.h>
#include <utils.h>
#include <handler.h>
#include <stdlib.h>
#include <tinyfiledialogs.h>
#include <backend.h>
#include <synth.h>
#include <threads.h>
#include <interface.h>
#include <utils.h>
#include <colors.h>



#define INTERPOLATION_DURATION 0.35


void updateWindowTitle(const char* title, int saved) {
    char* ttl = stringStrip(title);
    int length = strlen(ttl);

    if (!ttl || length==0) {
        free(ttl);
        SetWindowTitle("MidiLab");
    } else {
        const char* scndhlf = NULL;
        if (saved) scndhlf = " - MidiLab";
        else scndhlf = "* - MidiLab";
        char* out = concatenateStrings(ttl, scndhlf);
        free(ttl);
        SetWindowTitle(out);
        free(out);
    }
}


int windowShouldCloseDialog() {
    if (projectCanSafelyReplaceContents()) return 1;
    synthPanic();
    int result = tinyfd_messageBox("Warning", "Are you sure you want to close MidiLab?\nYour current project will be lost.", "yesno", "warning", 0);
    return result==1;    
}

int windowToggleFullscreenIfNecessary() {
    if (IsKeyPressed(KEY_F11)) {
        ToggleBorderlessWindowed();
        //ClearWindowState(FLAG_WINDOW_TOPMOST);
    }
    return 0;
}



static Color colorSetAlpha(Color c, unsigned char a) {
    c.a = a;
    return c;
}


static void _windowRenderBackgroundTask(float effect, const char* title, const char* description, float percentage, int totalJobs, int finishedJobs) {
    if (effect>1 || effect<=0) return;

    int jobsExist = (totalJobs>0 && totalJobs>=finishedJobs && finishedJobs>=0);
    int percentageExists = (percentage>=0 && percentage<=1);
    int percDivExists = (percentageExists || jobsExist);
    
    float titley=0.5, descy=0.5, percjobsy=0.5, percjobsh=0.06;

    if (title) {
        if (description && percDivExists) titley = 0.4;
        else if (description || percDivExists) titley=0.45;
        else titley = 0.5;
    }
    if (description) {
        if (title) descy = titley+0.1;
        else if (percDivExists) descy=0.45;
        else descy = 0.5;
    }
    if (percDivExists) {
        if (description) percjobsy=descy+0.09;
        else if (title) percjobsy=titley+0.1;
        else percjobsy=0.5;
    }

    float textSize1 = floatMin(screenSize.x, screenSize.y)*0.055;
    float textSize2 = 0.68*textSize1;
    unsigned char a1 = (unsigned char)(180*effect);
    unsigned char a2 = (unsigned char)(255*effect);

    DrawRectangleGradientH(0,0,(int)(screenSize.x),(int)(screenSize.y), (Color){13,24,28,a1}, (Color){25,17,35,a1});

    if (title) renderFontStringAlign(GlobalFonts[0].font, title, (Vector2){screenSize.x*0.5, screenSize.y*titley}, (Vector2){0.5,0.5}, textSize1, 0, colorSetAlpha(COLOR_TEXT_1, a2));
    if (description) renderFontStringAlign(GlobalFonts[0].font, description, (Vector2){screenSize.x*0.5, screenSize.y*descy}, (Vector2){0.5,0.5}, textSize2, 0, colorSetAlpha(COLOR_TEXT_3, a2));

    if (title && description) {
        float liny = screenSize.y*lerp(titley, descy, 0.4);
        Vector2 ttl =  textFontGetSize(GlobalFonts[0].font, title, textSize1, 0);
        DrawRectangleRounded(centerRectangle((Vector2){screenSize.x*0.5, liny}, (Vector2){ttl.x+4*interfaceSpace2, 0.006*screenSize.y}), 1.0, 4, colorSetAlpha(COLOR_TRACK_THEME_1, a2));
    }

    if (percDivExists) {
        Rectangle rect = centerRectangle((Vector2){screenSize.x*0.5, screenSize.y*percjobsy}, (Vector2){screenSize.x*0.6, screenSize.y*percjobsh});

        if (percentageExists) {
            Rectangle trect = rect;
            trect.width *= percentage;
            Color cl = colorSetAlpha(blendColors(COLOR_TRACK_THEME_3, (Color){0,0,0,255}, 0.15), a2);
            DrawRectangleRounded(trect, getRoundnessForRoundedRectangleTransformation(rect, trect, 0.5, 0), 8, cl);
            cl = colorSetAlpha(blendColors(COLOR_TRACK_THEME_5, (Color){255,255,255,255}, 0.25), a2);
            DrawRectangleRoundedLinesEx(rect, 0.5, 8, 0.004*screenSize.y, cl);
        }

        if (jobsExist) {
            renderFontStringAlign(GlobalFonts[0].font, TextFormat("%d/%d", finishedJobs, totalJobs), getRectangleCenter(rect), (Vector2){0.5,0.5}, textSize2, 0, colorSetAlpha(COLOR_TEXT_1, a2));
        }
    }
}



int windowRenderBackgroundProcess() {
    struct background_process prcs = threadGetCurrentBackgroundProcess();
    int activeProcess = threadIsThereActiveBackgroundProcess();
    double now = GetTime();

    //activeProcess = 1;
    //prcs = (struct background_process){.startT=0, .endT=0, .title="Export WAVE", .description="Exporting Wave to TestFile.wav...", .percentage=0.63, .totalJobs=48, .finishedJobs=30};

    if (!activeProcess && now-prcs.endT>=INTERPOLATION_DURATION) {
        if (prcs.title || prcs.description || (prcs.percentage<=1 && prcs.percentage>0) || prcs.totalJobs>0) threadClearOldProcessData();
        return 1;
    }

    
    float effect = 0;
    if (activeProcess) {
        effect = trigInterpolation(0, 1, (now-prcs.startT)/INTERPOLATION_DURATION);
    } else {
        float max = trigInterpolation(0, 1, (prcs.endT-prcs.startT)/INTERPOLATION_DURATION);
        effect = trigInterpolation(max, 0, (now-prcs.endT)/INTERPOLATION_DURATION);
    }

    _windowRenderBackgroundTask(effect, prcs.title, prcs.description, prcs.percentage, prcs.totalJobs, prcs.finishedJobs);
    return 0;
}