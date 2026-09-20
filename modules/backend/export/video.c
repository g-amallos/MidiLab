#include "export_internal.h"
#include <macros.h>
#include <stdio.h>
#include <raylib.h>
#include <visualizer.h>
#include <ffmpeg.h>
#include <string.h>


static int ffmpegAvailable = 0;

#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define VIDEO_FPS 60



typedef struct {
    volatile int videoExportLaunched;
    volatile int audioExportFinished;
    volatile Wave wave;
    volatile int sampleRate;
    volatile int fps;
    volatile int totalFrames;
    volatile int currentFrame;
    volatile double timestamp;
    volatile char* filepath;
    FFMPEG* ffmpeg;
} VideoExportState;


static volatile VideoExportState videoExportState = {.videoExportLaunched=0, .audioExportFinished=0, .wave={0}, .sampleRate=0, .fps=VIDEO_FPS, .totalFrames=0, .currentFrame=0, .timestamp=0, .filepath=NULL, .ffmpeg=0};


//static int _isFFmpegAvailable() {
//#ifdef IS_WINDOWS
//    FILE *pipe = _popen("ffmpeg -version > NUL 2>&1", "r");
//    if (!pipe) return false;
//    int result = _pclose(pipe);
//    return (result == 0);
//
//#elif defined(IS_LINUX)
//    FILE *pipe = popen("ffmpeg -version > /dev/null 2>&1", "r");
//    if (!pipe) return false;
//    int result = pclose(pipe);
//    return (result == 0);
//
//#else 
//    return 0;
//
//#endif
//}

int doesFFmpegExist();
int isFFmpegAvailable() {
    return ffmpegAvailable;
}


int exportVideoInit() {
    ffmpegAvailable = doesFFmpegExist();
    printf("FFmpeg: %s\n", ffmpegAvailable?"exists":"doesn't exist");

    return 0;
}





static int _generateRuntimeAudioFile() {
    const char* dir = "runtime/";
    if (!DirectoryExists(dir) && MakeDirectory(dir)) return 1;
    if (FileExists("runtime/audio.mp3")) FileRemove("runtime/audio.mp3");
    int ret = exportProjectAsMP3ForExportAll("runtime/audio.mp3", 1, 0);
    return ret;
}


static void _exportVideoStateInit(const char* filepath) {
    videoExportState.videoExportLaunched = 1;
    videoExportState.audioExportFinished = 0;
    videoExportState.wave = (Wave){0,};
    videoExportState.sampleRate = 0;
    videoExportState.fps = VIDEO_FPS;
    videoExportState.totalFrames = 0;
    videoExportState.currentFrame = 0;
    videoExportState.timestamp = 0;
    videoExportState.ffmpeg = NULL;
    videoExportState.filepath = strdup(filepath);
}

static void _exportVideoStateCloseThreadSafe() {
    videoExportState.videoExportLaunched = 0;
    videoExportState.audioExportFinished = 0;
    
    videoExportState.wave = (Wave){0,};
    videoExportState.sampleRate = 0;
    videoExportState.totalFrames = 0;
    videoExportState.currentFrame = 0;
    videoExportState.timestamp = 0;
    videoExportState.ffmpeg = NULL;
    if (videoExportState.filepath) free((char*)videoExportState.filepath);
    videoExportState.filepath = NULL;
}

static void _exportVideoStateCloseThreadUnsafe() {
    videoExportState.videoExportLaunched = 0;
    videoExportState.audioExportFinished = 0;
    if (videoExportState.wave.data) UnloadWave(videoExportState.wave);
    videoExportState.wave = (Wave){0,};
    videoExportState.sampleRate = 0;
    videoExportState.totalFrames = 0;
    videoExportState.currentFrame = 0;
    videoExportState.timestamp = 0;
    videoExportState.ffmpeg = NULL;
    if (videoExportState.filepath) free((char*)videoExportState.filepath);
    videoExportState.filepath = NULL;
    if (videoExportState.ffmpeg) ffmpeg_end_rendering(videoExportState.ffmpeg);
    videoExportState.ffmpeg = 0;
    FFTexportClose();
    visualizationRenderSimClose();
    SetTargetFPS(60);
}


static int _exportVideoStateWaveFinished() {
    if (!FileExists("runtime/audio.mp3")) {
        _exportVideoStateCloseThreadSafe();
        return 1;
    }
    videoExportState.currentFrame = 0;
    videoExportState.timestamp = 0;
    videoExportState.audioExportFinished = 1;
    return 0;
}

static int _exportVideoStateWaveFinishedFromMainThread(int resetOnError) {
    if (!FileExists("runtime/audio.mp3")) {
        if (resetOnError) _exportVideoStateCloseThreadUnsafe();
        return 1;
    }
    videoExportState.wave = LoadWave("runtime/audio.mp3");
    if (!(videoExportState.wave.data)) {
        _exportVideoStateCloseThreadUnsafe();
        return 1;
    }
    videoExportState.sampleRate = (int)(videoExportState.wave.sampleRate);
    videoExportState.totalFrames = videoExportState.fps*(1+videoExportState.wave.frameCount/(double)(videoExportState.wave.sampleRate));
    videoExportState.currentFrame = 0;
    videoExportState.timestamp = 0;
    if (FFTexportInit()) {
        _exportVideoStateCloseThreadUnsafe();
        return 1;
    }
    FFTexportZeroOutBuffers();
    return 0;
}

int exportVideoThreadFunction(const char* filepath) {
    if (!filepath) return 1;
    _exportVideoStateInit(filepath);
    int ret = _generateRuntimeAudioFile();
    if (ret) {
        _exportVideoStateCloseThreadSafe();
        return ret;
    }
    ret += _exportVideoStateWaveFinished();    
    return ret;
}

/*
ARCHIVE
int exportVideoCanRenderFrames() {
    if (videoExportState.videoExportLaunched!=1 || videoExportState.audioExportFinished!=1) return 0;
    if (videoExportState.currentFrame>=videoExportState.totalFrames) {
        if (videoExportState.ffmpeg) ffmpeg_end_rendering(videoExportState.ffmpeg);
        _exportVideoStateCloseThreadSafe();
        visualizationRenderSimClose();
        SetTargetFPS(60);
        return 0;
    } else if (videoExportState.currentFrame==0) {
        if (_exportVideoStateWaveFinishedFromMainThread(0)) return 0;
        printf("`exportVideoCanRenderFrames()`: file exists: %d\n", FileExists("runtime/audio.mp3"));
        if (FileExists("runtime/audio.mp3")) {
            visualizationRenderSimInit();
            videoExportState.ffmpeg = ffmpeg_start_rendering(VIDEO_WIDTH, VIDEO_HEIGHT, videoExportState.fps, "runtime/audio.mp3", (const char*)videoExportState.filepath);
            SetTargetFPS(180);
        } else return 0;
    }

    return 1;
}
*/



int exportVideoCanRenderFrames() {
    if (videoExportState.videoExportLaunched!=1 || videoExportState.audioExportFinished!=1) return 0;
    if (videoExportState.audioExportFinished==1) {
        if (!(videoExportState.wave.data) || !(videoExportState.totalFrames)) {
            if (_exportVideoStateWaveFinishedFromMainThread(0)) return 0;
            printf("`exportVideoCanRenderFrames()`: file exists: %d\n", FileExists("runtime/audio.mp3"));
            if (FileExists("runtime/audio.mp3")) {
                visualizationRenderSimInit();
                videoExportState.ffmpeg = ffmpeg_start_rendering(VIDEO_WIDTH, VIDEO_HEIGHT, videoExportState.fps, "runtime/audio.mp3", (const char*)videoExportState.filepath);
                SetTargetFPS(180);
            } else return 0;
        } else if (videoExportState.currentFrame>=videoExportState.totalFrames) {
            if (videoExportState.ffmpeg) ffmpeg_end_rendering(videoExportState.ffmpeg);
            _exportVideoStateCloseThreadUnsafe();
            visualizationRenderSimClose();
            SetTargetFPS(60);
            return 0;
        }
    }

    return 1;
}

int exportVideoThreadShouldClose(float* percentage, int* currentFrame, int* totalFrames) {
    if (videoExportState.videoExportLaunched!=1) {
        if (percentage) *percentage=1.0;
        if (totalFrames) *totalFrames=videoExportState.totalFrames;
        if (currentFrame) *currentFrame=videoExportState.totalFrames;
        return 1;
    }
    if (percentage) *percentage=videoExportState.currentFrame/(float)(videoExportState.totalFrames);
    if (totalFrames) *totalFrames=videoExportState.totalFrames;
    if (currentFrame) *currentFrame=videoExportState.currentFrame;
    return 0;
}

double exportVideoSimulationGetTime() {
    if (videoExportState.videoExportLaunched!=1 || videoExportState.audioExportFinished!=1 || !videoExportState.totalFrames || videoExportState.currentFrame>=videoExportState.totalFrames) return 0;
    return videoExportState.currentFrame/(double)videoExportState.fps;
}

double exportVideoSimulationGetDuration() {
    if (videoExportState.videoExportLaunched!=1 || videoExportState.audioExportFinished!=1 || !videoExportState.totalFrames || videoExportState.currentFrame>=videoExportState.totalFrames) return 0;
    return (videoExportState.totalFrames-1)/(double)videoExportState.fps;
}

double exportVideoSimulationGetAudioDuration() {
    if (videoExportState.videoExportLaunched!=1 || videoExportState.audioExportFinished!=1 || !videoExportState.totalFrames || videoExportState.currentFrame>=videoExportState.totalFrames || !(videoExportState.wave.data)) return 0;
    return videoExportState.wave.frameCount/(double)(videoExportState.wave.sampleRate);
}

void exportVideoVisSimulationFrame() {
    //printf("`exportVideoVisSimulationFrame`: entered\n");
    if (!videoExportState.wave.data) return;

    double dt=1.0/videoExportState.fps, time=videoExportState.timestamp;
    videoExportState.timestamp += dt;
    uint32_t sampleRate=videoExportState.sampleRate;//currentFrame=videoExportState.currentFrame, totalFrames=videoExportState.totalFrames;

    //printf("Rendering frame #%u / %u\n", currentFrame, totalFrames);

    double startT=time, endT=time+dt;
    uint32_t sampleTargetStart=(uint32_t)(startT*sampleRate), sampleTargetEnd=(uint32_t)(endT*sampleRate)-1;
    uint32_t sampleStart=sampleTargetStart, sampleEnd=sampleTargetEnd;
    if (sampleStart>=videoExportState.wave.frameCount) sampleStart=videoExportState.wave.frameCount-1;
    if (sampleEnd>=videoExportState.wave.frameCount) sampleEnd=videoExportState.wave.frameCount-1;

    float* buffer = ((float*)videoExportState.wave.data)+(sampleStart<<1);
    uint32_t samples=sampleEnd-sampleStart, silenceSamples=(sampleTargetEnd-sampleTargetStart)-(sampleEnd-sampleStart);

    if (samples) recordExportAudioFramesFloat(buffer, (int)samples);
    if (silenceSamples) recordExportAudioSilence((int)silenceSamples);

    float maxIntensity=0;
    FFTexportUpdate(&maxIntensity);
    visualizationRenderSimPrecomputeValues(maxIntensity);
    visualizationRenderSimRender();
    
    Image img = visualizationSimGetImage();
    ffmpeg_send_frame_flipped(videoExportState.ffmpeg, img.data, img.width, img.height);
    UnloadImage(img);

    (videoExportState.currentFrame)++;
    //printf("`exportVideoVisSimulationFrame`: finished\n");

}


void exportVideoBatchFrames(int frames) {
    //printf("`exportVideoBatchFrames`: frames=%d, videoExportLaunched=%d, audioExportFinished=%d\n", frames, videoExportState.videoExportLaunched, videoExportState.audioExportFinished);
    if (frames<=0 || videoExportState.videoExportLaunched!=1 || videoExportState.audioExportFinished!=1) return;

    for (int i=0; i<frames; i++) {
        if (exportVideoCanRenderFrames()) exportVideoVisSimulationFrame();
        else break;
    }
}