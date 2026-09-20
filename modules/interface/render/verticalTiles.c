#include <raylib.h>
#include <utils.h>
#include <interface.h>
#include <backend.h>
#include <ui.h>
#include <colors.h>
#include <stdio.h>


static Button backButton=NULL;
static Color tilesBackgroundColor={0,0,0,0}, disabledBackgroundColor={0,0,0,0}, textOrHoverBackgroundColor={0,0,0,0};


void verticalTilesInit() {
    Rectangle rect = {0,0,20,20};

    if (backButton) buttonFree(backButton);
    backButton = buttonCreate(rect, 0.25);

}

void verticalTilesClose() {
    if (backButton) buttonFree(backButton);
    backButton = NULL;
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
}

static void _updateTilesValues() {

    tilesBackgroundColor = blendColors(COLOR_BACKGROUND_3, COLOR_BACKGROUND_1, 0.3);
    disabledBackgroundColor = blendColors(tilesBackgroundColor, BLACK, 0.2);
    textOrHoverBackgroundColor = blendColors(tilesBackgroundColor, WHITE, 0.03);
}

void order2precomputeVerticalTiles() {
    _updateTilesValues();
    _updateTilesButtons();
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


static void _renderFeaturePlaceholder() {
    Rectangle rect1 = {0, 0, screenSize.x, controlLineHeight*1.2};
    DrawRectangleRec(rect1, tilesBackgroundColor);

    float textSize = 0.05*floatMin(screenSize.x, screenSize.y);
    renderFontStringAlign(GlobalFonts[0].font, "Settings Placeholder", getRectangleCenter(rect1), (Vector2){0.5,0.5}, 0.85*textSize, 0, COLOR_TEXT_1);

    float tx = 0.05*floatMin(screenSize.x, screenSize.y);
    Rectangle rect2 = {tx, rect1.y+rect1.height+tx, screenSize.x-2*tx, screenSize.y-rect1.y-rect1.height-2*tx};
    DrawRectangleRounded(rect2, 0.08, 8, tilesBackgroundColor);
    renderFontStringAlign(GlobalFonts[0].font, "Preview Placeholder", getRectangleCenter(rect2), (Vector2){0.5,0.5}, textSize, 0, COLOR_TEXT_1);
}



void renderVerticalTiles() {
    _renderFeaturePlaceholder();
    _renderBackButton();
}