#ifndef EXPORT_H
#define EXPORT_H


typedef struct track_data* Track;


int exportProjectTo(const char* filename);
int exportProjectToFilepath();
int importProjectFrom(const char* filename);

int exportProjectAsWave(const char* filename);

uint32_t estimateFileSizeForProject();

int exportProjectAsMidi(const char* filename);
int exportTrackAsMidi(const char* filename, Track track);

#endif