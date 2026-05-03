#include <interface.h>
#include <raymath.h>
#include <utils.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>



struct font_data GlobalFonts[2] = {
    {.title="JetBrains Mono (Regular)", .filepath="assets/fonts/JetBrainsMono/JetBrainsMono-Regular.ttf", .font={0}},
    {.title="JetBrains Mono (Light-Italic)", .filepath="assets/fonts/JetBrainsMono/JetBrainsMono-LightItalic.ttf", .font={0}}
};


int textFontInit() {
    int fontQuality = 128;
    int initFonts=0;
    int fontsNum=sizeof(GlobalFonts)/sizeof(struct font_data);
    for (int i=0; i<fontsNum; i++) {
        GlobalFonts[i].font = LoadFontEx(GlobalFonts[i].filepath, fontQuality, NULL, 0);
        if (GlobalFonts[i].font.texture.id>0) initFonts++;
        else {
            TraceLog(LOG_WARNING, "FONT: [%s] could not be loaded", GlobalFonts[i].filepath);
            printf("FONT: [%s] could not be loaded\n", GlobalFonts[i].filepath);
        }
    }
    return initFonts;
}

int textFontClose() {
    int closedFonts=0;
    int fontsNum=sizeof(GlobalFonts)/sizeof(struct font_data);
    for (int i=0; i<fontsNum; i++) {
        if (GlobalFonts[i].font.texture.id>0) {
            UnloadFont(GlobalFonts[i].font);
            GlobalFonts[i].font = (Font){ 0 };
            closedFonts++;
        }
    }
    return closedFonts;
}


void renderFontStringAlign(Font font, const char* string, Vector2 pos, Vector2 align, float size, float spacing, Color color) {
    Vector2 final = pos;
    if (align.x!=0 || align.y!=0) {
        Vector2 width = MeasureTextEx(font, string, size, spacing);
        final = lerpVector2_vec(pos, Vector2Subtract(pos, width), align);
    }
    DrawTextEx(font, string, final, size, spacing, color);
}

