#include <dlls.h>
#include <stdio.h>


#ifdef IS_WINDOWS
    #include <direct.h>
#endif

#ifdef IS_LINUX
    #include <time.h>
    #include <unistd.h>
#endif



void removeDirectory(const char* dir) {
    #ifdef IS_WINDOWS
    _rmdir(dir);
    #else
    rmdir(dir);
    #endif
}

void sleepMS(long ms) {
    #ifdef IS_WINDOWS
    Sleep((DWORD)ms);
    
    #else
    struct timespec ts;
    ts.tv_sec = ms/1000;
    ts.tv_nsec = (ms%1000)*1000000L;
    nanosleep(&ts, NULL);
    
    #endif
}

int doesFFmpegExist() {
#ifdef IS_WINDOWS
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    char cmd[] = "ffmpeg -version";

    BOOL success = CreateProcessA(
        NULL,
        cmd,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!success) return 0;

    WaitForSingleObject(pi.hProcess, 1000);

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (exitCode == 0);

#elif defined(IS_LINUX)
    return 0;
    FILE *pipe = popen("ffmpeg -version > /dev/null 2>&1", "r");
    if (!pipe) return false;
    int result = pclose(pipe);
    return (result == 0);
#else 
    return 0;
#endif
}