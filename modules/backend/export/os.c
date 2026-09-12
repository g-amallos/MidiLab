#include <dlls.h>
#include <stdio.h>


#ifdef IS_WINDOWS
    #include <direct.h>
#endif

#ifdef IS_LINUX
    #include <unistd.h>
#endif



void removeDirectory(const char* dir) {
    #ifdef IS_WINDOWS
    _rmdir(dir);
    #else
    rmdir(dir);
    #endif
}