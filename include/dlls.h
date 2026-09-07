#ifndef DLLS_H
#define DLLS_H



#if defined(_WIN32) || defined(_WIN64)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    #define IS_WINDOWS

#elif defined(__linux__)

    #define IS_LINUX

#else
    #error "Platform not supported"
#endif


#endif