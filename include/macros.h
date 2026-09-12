#ifndef MACROS_H
#define MACROS_H


#if defined(_WIN32) || defined(_WIN64)
    #define IS_WINDOWS

#elif defined(__linux__)
    #define IS_LINUX

#else
    #error "Platform not supported"
#endif


#ifdef DEBUG
    #define IS_DEBUG 1
#else
    #define IS_DEBUG 0
#endif




#endif