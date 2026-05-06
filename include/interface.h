#ifndef INTERFACE_H
#define INTERFACE_H

#include <raylib.h>




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
void renderRectangleCentered(Vector2 pos, Vector2 dim, Color col);
void renderRoundedRectangleCentered(Vector2 pos, Vector2 dim, Color col, float roundness, int segments);
void renderRoundedRectangleLinesCentered(Vector2 pos, Vector2 dim, Color col, float roundness, int segments, float thickness);
int checkCollisionPointRoundedRect(Vector2 point, Rectangle rect, float roundness);
float getRadiusForRoundedRectangle(Rectangle rect, float roundness);
float getRoundnessForRoundedRectangle(Rectangle rect, float radius);




/* Render (interface/render/) */

/* Backgrounds (interface/render/backgrounds.c) */

void renderMainBackground();        // Renders the main background



/* Control Line (interface/render/controlLine.c) */

void controlLineInit();
void controlLineClose();
void renderControlLine();



/* Colors (here) */




#endif