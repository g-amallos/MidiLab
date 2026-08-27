#ifndef INTERFACE_H
#define INTERFACE_H

#include <raylib.h>
#include <backend.h>



typedef struct ui_button *Button;



extern float interfaceSpace1;
extern float interfaceSpace2;


/* General (interface/general.c) */

int renderInit();                   // Initialized the interface
int renderClose();                  // Unloads from the memory anything related to (initialized from) the interface
int updateRenderGlobalVariables();  // Update global values needed to other functions before rendering
int render();                       // Runs the render iteration

extern Vector2 screenSize;


/* Fonts (interface/textfont.c) */

struct font_data {
    const char* title;
    const char* filepath;
    Font font;
};

extern struct font_data GlobalFonts[2];

int textFontInit();     // Returns the number of loaded fonts
int textFontClose();    // Returns the number of unloaded fonts

void renderFontStringAlign(Font font, const char* string, Vector2 pos, Vector2 align, float size, float spacing, Color color);
Vector2 textFontGetSize(Font font, const char* string, float size, float spacing);


/* Rectangles (interface/rectangles.c) */

Rectangle centerRectangle(Vector2 pos, Vector2 dim);
Vector2 getRectangleCenter(Rectangle rect);
Rectangle scaleRctangleFromCenter(Rectangle rect, float scale);
Rectangle scaleRctangleFromCenterV(Rectangle rect, Vector2 scale);
Rectangle alignRectangle(Vector2 pos, Vector2 dim, Vector2 align);
Rectangle rectangleMoveToFitInsideRect(Rectangle interior, Rectangle exterior);
Rectangle rectangleScaleToFitInCenter(Vector2 originalDimensions, Rectangle toFit);
Rectangle rectangleAnd(Rectangle rec1, Rectangle rec2);
void renderRectangleCentered(Vector2 pos, Vector2 dim, Color col);
void renderRoundedRectangleCentered(Vector2 pos, Vector2 dim, Color col, float roundness, int segments);
void renderRoundedRectangleLinesCentered(Vector2 pos, Vector2 dim, Color col, float roundness, int segments, float thickness);
int checkCollisionPointRoundedRect(Vector2 point, Rectangle rect, float roundness);
float getRadiusForRoundedRectangle(Rectangle rect, float roundness);
float getRoundnessForRoundedRectangle(Rectangle rect, float radius);
Rectangle rectangleClip(Rectangle source, Rectangle clip);




/* Render (interface/render/) */

void renderFPS();

/* Backgrounds (interface/render/backgrounds.c) */

void renderMainBackground();        // Renders the main background



/* Control Line (interface/render/controlLine.c) */

extern float controlLineHeight;
extern float buttonList4x5ExampleSpacing;
extern Rectangle buttonList4x5ExampleRect;
void controlLineInit();
void controlLineClose();
void renderControlLine();
void order1PrecomputeControlLine();
void order2PrecomputeControlLine();

void exportProjectToSavedFilepath();
void exportProjectByCtrlS();




/* Tracks Left (interface/render/tracks.c) */

extern float trackCLineHeight;
extern float trackHeight;
extern float trackLeftWidth;
extern float trackDivTargetHeight;
extern Color trackThemeColors[7];
extern Color programTypeColors[MPT_END];
extern int timelineMeasureSkipsTop, timelineMeasureSkipsBottom, timelineBeatsSkipsTop, timelineBeatsSkipsBottom;
extern Button timeLineDragButton;

float normalizeProgramTypeIcon(enum icon_title iconType);
void createTrackUIsFromScratch(uint8_t* colArr);

void renderTracksLeftInit();
void renderTracksLeftClose();
void renderTrackCreateNew();
void renderTrackDeleteAtIdx(int idx);
int getTrackThemeColorIdx(int i);
Color getTrackThemeColor(int i);
Color getSelectedTrackThemeColor();
Color getTrackThemeColorForWhiteKeys();
Color getTrackThemeColorForBlackKeys();
void renderTracksLeft();
void order1PrecomputeTracksLeft();
void order2PrecomputeTracksLeft();
void renderTracksLeftLayoutsIfAny();





/* Bottom Half Layout (interface/render/bottomHalfLayout.c) */

extern float bottomHalfHeight;
extern float bottomHalfUsefulHeight;
extern float bottomHalfLayoutTopPadding;
extern Color bottomHalfBackgroundColor;

void bottomHalfLayoutInit();
void bottomHalfLayoutClose();
void order1PrecomputeBottomHalfLayout();
void order2PrecomputeBottomHalfLayout();
void renderBottomHalfLayout();
void updateBottomHalfSizeOnResize(Vector2 oldScreen, Vector2 newScreen);




/* Horizontal Keyboard (interface/render/horizontalKeyboard.c) */

extern float horizontalKeyboardHeight;

void horizontalKeyboardInit();
void horizontalKeyboardClose();
void precalculateSizesHorizontalKeyboard();
void renderHorizontalKeyboard();                // Must have already ran `precalculateSizesHorizontalKeyboard()`
void precalculateJustHorizontalKeyboard();




/* Vertical Keyboard (interface/render/verticalKeyboard.c) */

void verticalKeyboardInit();
void verticalKeyboardClose();
void preCalculateNecessaryVerticalKeyboard();
void preCalculateVerticalKeyboard();
void renderVerticalKeyboard();
void renderWholeBottomLayoutTypeVertical();


/* Layout (interface/render/layout.c)  (More of a test) */

void renderLayoutLines();




/* Instrument Picker (interface/render/instrumentPicker.c) */

void createInstrumentPicker();
void destroyInstrumentPicker();
void precalculateInstrumentPicker();
void renderInstrumentPicker();

/* Colors (here) */




#endif