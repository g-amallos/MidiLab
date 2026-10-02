#ifndef WATERFALL_H
#define WATERFALL_H

#include <raylib.h>



void verticalTilesVideoExportInit();
void verticalTilesVideoExportClose();


void tilesSettingsGetResolution(int* width, int* height);
double tilesSettingsGetTotalDuration();
void tilesSettingsGetDelays(double* startDelay, double* endDelay);


void tilesVideoExportRenderFrame();
Image tilesGetImage();


#endif