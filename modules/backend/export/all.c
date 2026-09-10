#include "export_internal.h"
#include <utils.h>
#include <raylib.h>
#include <string.h>

#define DEFAULT_PROJECT_TITLE "Untitled Project"
#define DEFAULT_TRACK_TITLE "New Track"





static char* _generateStringFilepathFor(const char* directory, const char* extension) {
    const char* projectTitle = projectGetCurrentTitle();
    if (!projectTitle) projectTitle = DEFAULT_PROJECT_TITLE;

    char* title = stringToFileName(projectTitle, 30);
    char* conct = concatenateStrings4(directory, "/", title, extension);
    printf("Path: %s | File: %s\n", directory, conct);
    free(title);
    return conct;
}

static int _exportProjectToDirectory(const char* directory) {
    if (!directory) return 1;
    
    char* fullpath = _generateStringFilepathFor(directory, ".mlb");
    FILE* fptr = fopen(fullpath, "wb");
    free(fullpath);
    if (!fptr) return 1;

    int ret = writeProject(fptr);
    fclose(fptr);
    return ret;
}

static int _exportWaveToDirectory(const char* directory) {
    if (!directory) return 1;
    
    char* fullpath = _generateStringFilepathFor(directory, ".wav");
    int ret = exportProjectAsWave(fullpath);
    free(fullpath);
    return ret;
}

static int _exportMidiToDirectory(const char* directory) {
    if (!directory) return 1;
    
    char* fullpath = _generateStringFilepathFor(directory, ".mid");
    int ret = exportProjectAsMidi(fullpath);
    free(fullpath);
    return ret;
}


static void removeDirectoryRecursively(const char *dirPath) {
    if (!DirectoryExists(dirPath)) return;
    FilePathList files = LoadDirectoryFiles(dirPath);

    for (unsigned int i=0; i<files.count; i++) {
        const char *path = files.paths[i];
        const char *filename = GetFileName(path);
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) continue;
        if (DirectoryExists(path)) removeDirectoryRecursively(path);
        else remove(path);
    }

    UnloadDirectoryFiles(files);
    removeDirectory(dirPath); 
}

/*
static int _prepareDirectory(const char* directory) {
    if (!DirectoryExists(directory)) return MakeDirectory(directory);
    return 0;
}*/

static char* _setupDirectoryConversion(const char* directory) {
    char* dir = strdup(TextReplace(directory, "\\", "/"));
    if (!dir) return NULL;
    if (DirectoryExists(dir)) removeDirectoryRecursively(dir);
    if (MakeDirectory(dir)) return NULL;
    return dir;
}

static int _exportTrackAllTo(const char* dir, int idx) {
    if (!globalProject || idx<0 || idx>=globalProject->tracksNum) return 1;

    char* title = stringToFileName((globalProject->tracks)[idx].title, 30);
    if (!title) return 1;
    char* ndir = strdup(TextFormat("%s/%d.%s", dir, idx+1, title));
    if (!ndir) {
        free(title);
        return 2;
    }
    char* newDir = _setupDirectoryConversion(ndir);
    free(ndir);
    if (!newDir) {
        free(title);
        return 3;
    }

    int ret=0;
    ret += exportTrackTo(TextFormat("%s/%s.mlt", newDir, title), idx);
    ret += exportTrackAsMidi(TextFormat("%s/%s.mid", newDir, title), globalProject->tracks+idx);

    free(newDir);
    free(title);

    return ret;
}

static int _exportTracks(const char* directory) {
    if (!globalProject || !(globalProject->tracksNum) || !(globalProject->tracks)) return 1;

    int num = globalProject->tracksNum;
    char* tracksDir = _setupDirectoryConversion(concatenateStrings(directory, "/Tracks"));
    if (!tracksDir) return 1;

    int ret=0;
    for (int i=0; i<num; i++) {
        ret += _exportTrackAllTo(tracksDir, i);
    }
    free(tracksDir);
    return ret;
}

int exportAll(const char* directory) {
    if (!directory || !globalProject) return 1;
    char* dir = _setupDirectoryConversion(directory);
    if (!dir) return 1;

    int ret=0;
    ret += _exportProjectToDirectory(dir);
    ret += _exportWaveToDirectory(dir);
    ret += _exportMidiToDirectory(dir);
    ret += _exportTracks(dir);

    free(dir);

    return ret;
}