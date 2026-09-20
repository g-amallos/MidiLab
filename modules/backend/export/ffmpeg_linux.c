#include <stdio.h>
#include <stdint.h>


typedef struct {
    int nothing;
} FFMPEG;



FFMPEG* ffmpeg_start_rendering(size_t width, size_t height, size_t fps, const char* soundFilepath, const char* videoFilepath) {
    (void)soundFilepath;
    (void)videoFilepath;
    (void)fps;
    (void)width;
    (void)height;
    return NULL;
}

void ffmpeg_send_frame(FFMPEG *ffmpeg, void *data, size_t width, size_t height) {
    (void)ffmpeg;
    (void)data;
    (void)width;
    (void)height;
    return;
}

void ffmpeg_send_frame_flipped(FFMPEG *ffmpeg, void *data, size_t width, size_t height) {
    (void)ffmpeg;
    (void)data;
    (void)width;
    (void)height;
    return;
}

void ffmpeg_end_rendering(FFMPEG *ffmpeg) {
    (void)ffmpeg;
    return;
}