#ifndef THREADS_H
#define THREADS_H


struct background_process {
    char* title;
    char* description;
    float percentage;
    int totalJobs;
    int finishedJobs;
    double startT;
    double endT;
};


//extern volatile struct background_process backgroundProcess;


int threadsClose();

int threadClearOldProcessData();
int threadIsThereActiveBackgroundProcess();
struct background_process threadGetCurrentBackgroundProcess();
int threadEditProcess(const char* title, const char* description, float percentage, int totalJobs, int finishedJobs);   // Allowed only by the specific worker thread
int threadEditProcessPercentage(float percentage);                                                          // Allowed only by the specific worker thread
int threadEditProcessDescription(const char* description);                                                        // Allowed only by the specific worker thread


int threadRequestExportWave(const char* filename);
int threadRequestExportMP3(const char* filename);
int threadRequestExportAll(const char* directory);
int threadRequestExportVideo(const char* filename);


#endif