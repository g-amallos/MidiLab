#include <pthread.h>
#include <threads.h>
#include <stdlib.h>
#include <export.h>
#include <stdio.h>
#include <string.h>
#include <raylib.h>
#include <platform.h>
#include <utils.h>



struct background_thread {
    volatile struct background_process process;
    volatile int active;
    pthread_t thread;
};



//volatile struct background_process backgroundProcess={NULL,0,0,0};
static volatile struct background_thread backgroundThread={0};



static void _clearBackgroundProcess() {
    if (backgroundThread.process.title) free(backgroundThread.process.title);
    if (backgroundThread.process.description) free(backgroundThread.process.description);
    backgroundThread.process.title = NULL;
    backgroundThread.process.description = NULL;
    backgroundThread.process.percentage = 0;
    backgroundThread.process.totalJobs = 0;
    backgroundThread.process.finishedJobs = 0;
}


static void _clearBackgroundThread() {
    if (!(backgroundThread.active)) return;

    pthread_t callerThread = pthread_self();
    if (!pthread_equal(callerThread, backgroundThread.thread)) return;

    backgroundThread.active = 0;
    backgroundThread.process.endT = GetTime();
}

int threadClearOldProcessData() {
    if (threadIsThereActiveBackgroundProcess()) return 1;
    _clearBackgroundProcess();
    return 0;
}

int threadsClose() {
    return threadClearOldProcessData();
}

int threadIsThereActiveBackgroundProcess() {
    return backgroundThread.active;
}

struct background_process threadGetCurrentBackgroundProcess() {
    return backgroundThread.process;
}



static int _threadCanCreateNewThread() {
    return !(threadIsThereActiveBackgroundProcess());
}

static int _initThread(char* title, void* (*func)(void*), void* args) {
    if (!(_threadCanCreateNewThread())) {
        if (title) free(title);
        if (args) free(args);
        return 1;
    }

    backgroundThread.active = 1;
    backgroundThread.process.title = title;
    backgroundThread.process.startT = GetTime();
    pthread_t thread;
    if (pthread_create(&thread, NULL, func, args) != 0) {
        _clearBackgroundProcess();
        if (args) free(args);
        return 1;
    }
    backgroundThread.thread = thread;
    pthread_detach(thread);
    return 0;
}

int threadEditProcess(const char* title, const char* description, float percentage, int totalJobs, int finishedJobs) {
    pthread_t callerThread = pthread_self();
    if (!pthread_equal(callerThread, backgroundThread.thread)) return 1;

    if (title) {
        char* oldlTitle = backgroundThread.process.title;
        backgroundThread.process.title = strdup(title);
        if (oldlTitle) free(oldlTitle);
    }

    if (description) {
        char* oldlDscr = backgroundThread.process.description;
        backgroundThread.process.description = strdup(description);
        if (oldlDscr) free(oldlDscr);
    }

    backgroundThread.process.percentage = percentage;
    backgroundThread.process.totalJobs = totalJobs;
    backgroundThread.process.finishedJobs = finishedJobs;

    return 0;
}

int threadEditProcessPercentage(float percentage) {
    pthread_t callerThread = pthread_self();
    if (!pthread_equal(callerThread, backgroundThread.thread)) return 1;
    backgroundThread.process.percentage = percentage;
    return 0;
}

int threadEditProcessDescription(const char* description) {
    pthread_t callerThread = pthread_self();
    if (!pthread_equal(callerThread, backgroundThread.thread)) return 1;
    if (description) {
        char* oldlDscr = backgroundThread.process.description;
        backgroundThread.process.description = strdup(description);
        if (oldlDscr) free(oldlDscr);
    } else {
        char* oldlDscr = backgroundThread.process.description;
        backgroundThread.process.description = NULL;
        if (oldlDscr) free(oldlDscr);
    }
    return 0;
}


static void* _exportAll(void* args) {
    int ret = exportAll((const char*)args);
    printf("EXPORT ALL: %d\n", ret);
    
    if (args) free(args);
    _clearBackgroundThread();
    return NULL;
}

static void* _exportWave(void* args) {
    threadEditProcess(NULL, NULL, 0.0, 0, 0);
    int ret = exportProjectAsWave((const char*)args);
    printf("EXPORT WAVE: %d\n", ret);
    
    if (args) free(args);
    _clearBackgroundThread();
    return NULL;
}

static void* _exportMP3(void* args) {
    threadEditProcess(NULL, NULL, 0.0, 0, 0);
    int ret = exportProjectAsMP3((const char*)args);
    printf("EXPORT MP3: %d\n", ret);
    
    if (args) free(args);
    _clearBackgroundThread();
    return NULL;
}

static void* _exportVideo(void* args) {
    threadEditProcess(NULL, NULL, 0.0, 0, 0);
    threadEditProcessDescription("Generating Audio...");
    int ret = exportVideoThreadFunction((char*)args);
    printf("EXPORT VIDEO: %d\n", ret);
    threadEditProcessDescription("Rendering Frames...");
    if (ret) {
        if (args) free(args);
        _clearBackgroundThread();
        return NULL;
    }

    int totalJobs=0, currentJob=0;
    float perc=0;
    while (!exportVideoThreadShouldClose(&perc, &currentJob, &totalJobs)) {
        threadEditProcess(NULL, NULL, perc, totalJobs, currentJob);
        sleepMS(100);
    }
    
    printf("_exportVideo thread finished\n");
    if (args) free(args);
    _clearBackgroundThread();
    return NULL;
}


int threadRequestExportAll(const char* directory) {
    if (!(_threadCanCreateNewThread())) return 1;

    return _initThread(strdup("Export All"), _exportAll, strdup(directory));
}

int threadRequestExportWave(const char* filename) {
    if (!(_threadCanCreateNewThread())) return 1;

    return _initThread(strdup("Export Wave"), _exportWave, strdup(filename));
}

int threadRequestExportMP3(const char* filename) {
    if (!(_threadCanCreateNewThread())) return 1;

    return _initThread(strdup("Export MP3"), _exportMP3, strdup(filename));
}

int threadRequestExportVideo(const char* filename) {
    if (!(_threadCanCreateNewThread())) return 1;

    return _initThread(strdup("Export Video"), _exportVideo, strdup(filename));
}