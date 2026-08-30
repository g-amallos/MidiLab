#ifndef EXPORT_H
#define EXPORT_H


int exportProjectTo(const char* filename);
int exportProjectToFilepath();
int importProjectFrom(const char* filename);

int exportProjectAsWave(const char* filename);

uint32_t estimateFileSizeForProject();

#endif