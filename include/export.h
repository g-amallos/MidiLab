#ifndef EXPORT_H
#define EXPORT_H

#include <stdint.h>


typedef struct track_data* Track;


int exportProjectTo(const char* filename);
int exportProjectToFilepath();
int importProjectFrom(const char* filename);
int importTrackFrom(const char* filename, int idx);
int exportTrackTo(const char* filename, int idx);
int exportAll(const char* directory);

int exportProjectAsWave(const char* filename);
int exportProjectAsMP3(const char* filename);

uint32_t estimateFileSizeForProject();

int exportProjectAsMidi(const char* filename);
int exportTrackAsMidi(const char* filename, Track track);



int isFFmpegAvailable();
int exportVideoInit();

int exportVideoFFTThreadFunction(const char* filepath);
int exportVideoWaterfallThreadFunction(const char* filepath);
void exportVideoBatchFrames(int frames);
int exportVideoThreadShouldClose(float* percentage, int* currentFrame, int* totalFrames);
double exportVideoSimulationGetTime();
double exportVideoSimulationGetDuration();
double exportVideoSimulationGetAudioDuration();


#endif