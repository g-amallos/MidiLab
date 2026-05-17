#include <interface.h>
#include <raymath.h>
#include <utils.h>



Rectangle centerRectangle(Vector2 pos, Vector2 dim) {
    return (Rectangle){.x=pos.x-0.5*dim.x, .y=pos.y-0.5*dim.y, .width=dim.x, .height=dim.y};
}

Vector2 getRectangleCenter(Rectangle rect) {
    return (Vector2){.x=rect.x+0.5*rect.width, .y=rect.y+0.5*rect.height};
}

Rectangle scaleRctangleFromCenter(Rectangle rect, float scale) {
    Vector2 center = {rect.x+0.5*rect.width, rect.y+0.5*rect.height};
    return (Rectangle){.x=center.x-0.5*scale*rect.width, .y=center.y-0.5*scale*rect.height, .width=scale*rect.width, .height=scale*rect.height};
}

Rectangle scaleRctangleFromCenterV(Rectangle rect, Vector2 scale) {
    Vector2 center = {rect.x+0.5*rect.width, rect.y+0.5*rect.height};
    return (Rectangle){.x=center.x-0.5*scale.x*rect.width, .y=center.y-0.5*scale.y*rect.height, .width=scale.x*rect.width, .height=scale.y*rect.height};
}

Rectangle alignRectangle(Vector2 pos, Vector2 dim, Vector2 align) {
    return (Rectangle){.x=pos.x-align.x*dim.x, .y=pos.y-align.y*dim.y, .width=dim.x, .height=dim.y};
}

Rectangle rectangleMoveToFitInsideRect(Rectangle interior, Rectangle exterior) {
    if (interior.width>exterior.width || interior.height>exterior.height) return interior;
    if (interior.y<exterior.y) interior.y=exterior.y;
    if (interior.y+interior.height>exterior.y+exterior.height) interior.y=exterior.y+exterior.height-interior.height;
    if (interior.x<exterior.x) interior.x=exterior.x;
    if (interior.x+interior.width>exterior.x+exterior.width) interior.x=exterior.x+exterior.width-interior.width;
    return interior;
}

Rectangle rectangleScaleToFitInCenter(Vector2 originalDimensions, Rectangle toFit) {
    if (originalDimensions.x<=0 || originalDimensions.y<=0) return (Rectangle){0,0,0,0};
    if (toFit.height/originalDimensions.y<toFit.width/originalDimensions.x) {
        float w2 = originalDimensions.x/originalDimensions.y*toFit.height;
        return (Rectangle){toFit.x+0.5*(toFit.width-w2), toFit.y, w2, toFit.height};
    } else {
        float h2 = originalDimensions.y/originalDimensions.x*toFit.width;
        return (Rectangle){toFit.x, toFit.y+0.5*(toFit.height-h2), toFit.width, h2};
    }
}

void renderRectangleCentered(Vector2 pos, Vector2 dim, Color col) {
    Vector2 newPos = Vector2Add(pos, Vector2Scale(dim, -0.5));
    DrawRectangleV(newPos, dim, col);
}

void renderRoundedRectangleCentered(Vector2 pos, Vector2 dim, Color col, float roundness, int segments) {
    DrawRectangleRounded(centerRectangle(pos, dim), roundness, segments, col);
}

void renderRoundedRectangleLinesCentered(Vector2 pos, Vector2 dim, Color col, float roundness, int segments, float thickness) {
    DrawRectangleRoundedLinesEx(centerRectangle(pos, dim), roundness, segments, thickness, col);
}

int checkCollisionPointRoundedRect(Vector2 point, Rectangle rect, float roundness) {
    if (!CheckCollisionPointRec(point, rect)) return 0;

    float radius = 0.5*roundness*floatMin(rect.width, rect.height);

    Rectangle innerRectX = {rect.x+radius, rect.y, rect.width-2*radius, rect.height};
    Rectangle innerRectY = {rect.x, rect.y+radius, rect.width, rect.height-2*radius};

    if (CheckCollisionPointRec(point, innerRectX) || CheckCollisionPointRec(point, innerRectY)) return 1;

    if (CheckCollisionPointCircle(point, (Vector2){rect.x+radius, rect.y+radius}, radius)) return 1;
    if (CheckCollisionPointCircle(point, (Vector2){rect.x+rect.width-radius, rect.y+radius}, radius)) return 1;
    if (CheckCollisionPointCircle(point, (Vector2){rect.x+radius, rect.y+rect.height-radius}, radius)) return 1;
    if (CheckCollisionPointCircle(point, (Vector2){rect.x+rect.width-radius, rect.y+rect.height-radius}, radius)) return 1;

    return 0;
}

float getRadiusForRoundedRectangle(Rectangle rect, float roundness) {
    return 0.5*roundness*floatMin(rect.width, rect.height);
}

float getRoundnessForRoundedRectangle(Rectangle rect, float radius) {
    if (radius<=0) return 0;
    float md = floatMin(rect.width, rect.height);
    if (md<=0) return 0;
    return 2*radius/md;
}