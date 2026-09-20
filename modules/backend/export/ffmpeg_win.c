// https://github.com/tsoding/rendering-video-in-c-with-ffmpeg/blob/master/ffmpeg_windows.c

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct {
    HANDLE hProcess;
    // HANDLE hPipeRead;
    HANDLE hPipeWrite;
} FFMPEG;

static LPSTR GetLastErrorAsString(void) {
    // https://stackoverflow.com/questions/1387064/how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror

    DWORD errorMessageId = GetLastError();
    assert(errorMessageId != 0);

    LPSTR messageBuffer = NULL;

    FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, // DWORD   dwFlags,
            NULL, // LPCVOID lpSource,
            errorMessageId, // DWORD   dwMessageId,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // DWORD   dwLanguageId,
            (LPSTR) &messageBuffer, // LPTSTR  lpBuffer,
            0, // DWORD   nSize,
            NULL // va_list *Arguments
        );

    return messageBuffer;
}

FFMPEG *ffmpeg_start_rendering(size_t width, size_t height, size_t fps, const char* soundFilepath, const char* videoFilepath) {
    HANDLE pipe_read;
    HANDLE pipe_write;

    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    if (!CreatePipe(&pipe_read, &pipe_write, &saAttr, 0)) {
        return NULL;
    }

    if (!SetHandleInformation(pipe_write, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(pipe_read);
        CloseHandle(pipe_write);
        return NULL;
    }

    // Open NUL device to swallow FFmpeg stdout/stderr in GUI mode (-mwindows)
    HANDLE hNul = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &saAttr, OPEN_EXISTING, 0, NULL);

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdInput = pipe_read;
    siStartInfo.hStdOutput = hNul;
    siStartInfo.hStdError = hNul;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    char cmd_buffer[1024*2];
    snprintf(cmd_buffer, sizeof(cmd_buffer), 
        "ffmpeg.exe -loglevel quiet -y -f rawvideo -pix_fmt rgba -s %dx%d -r %d -i - -i \"%s\" -c:v libx264 -vb 2500k -c:a aac -ab 200k -pix_fmt yuv420p \"%s\"", 
        (int)width, (int)height, (int)fps, soundFilepath, videoFilepath);

    // Passed CREATE_NO_WINDOW to hide the console completely
    BOOL bSuccess = CreateProcess(
        NULL,
        cmd_buffer,
        NULL,
        NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &siStartInfo,
        &piProcInfo
    );

    // Close the read handle and NUL handle in main process (child process inherited them)
    CloseHandle(pipe_read);
    if (hNul != INVALID_HANDLE_VALUE) {
        CloseHandle(hNul);
    }

    if (!bSuccess) {
        CloseHandle(pipe_write);
        return NULL;
    }

    CloseHandle(piProcInfo.hThread);

    FFMPEG *ffmpeg = (FFMPEG*)malloc(sizeof(FFMPEG));
    if (!ffmpeg) return NULL;

    ffmpeg->hProcess = piProcInfo.hProcess;
    ffmpeg->hPipeWrite = pipe_write;
    return ffmpeg;
}

void ffmpeg_send_frame(FFMPEG *ffmpeg, void *data, size_t width, size_t height) {
    WriteFile(ffmpeg->hPipeWrite, data, sizeof(uint32_t)*width*height, NULL, NULL);
}

void ffmpeg_send_frame_flipped(FFMPEG *ffmpeg, void *data, size_t width, size_t height) {
    for (size_t y = height; y > 0; --y) {
        WriteFile(ffmpeg->hPipeWrite, (uint32_t*)data + (y - 1)*width, sizeof(uint32_t)*width, NULL, NULL);
    }
}

void ffmpeg_end_rendering(FFMPEG *ffmpeg) {
    FlushFileBuffers(ffmpeg->hPipeWrite);
    CloseHandle(ffmpeg->hPipeWrite);
    DWORD result = WaitForSingleObject(
                       ffmpeg->hProcess,     // HANDLE hHandle,
                       INFINITE // DWORD  dwMilliseconds
                   );

    if (result == WAIT_FAILED) {
        fprintf(stderr, "ERROR: could not wait on child process: %s\n", GetLastErrorAsString());
        return;
    }

    DWORD exit_status;
    if (GetExitCodeProcess(ffmpeg->hProcess, &exit_status) == 0) {
        fprintf(stderr, "ERROR: could not get process exit code: %lu\n", GetLastError());
        return;
    }

    if (exit_status != 0) {
        fprintf(stderr, "ERROR: command exited with exit code %lu\n", exit_status);
        return;
    }

    CloseHandle(ffmpeg->hProcess);
}