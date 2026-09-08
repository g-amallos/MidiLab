#include <dlls.h>
#include <stdio.h>


#ifdef IS_WINDOWS

    void print_dll_location(const char* dll_name) {
        HMODULE hModule = GetModuleHandle(dll_name);
        if (!hModule) {
            printf("DLL '%s' is not loaded in this process.\n", dll_name);
            return;
        }

        char path[MAX_PATH];
        if (GetModuleFileName(hModule, path, MAX_PATH)>0) printf("Loaded '%s' from: %s\n", dll_name, path);
        else printf("Failed to retrieve path. Error code: %lu\n", GetLastError());
    }

    void check_dll() {
        print_dll_location(NULL);
        print_dll_location("libfluidsynth-3.dll");
        int succeeded = SetDllDirectory("bin\\libs");
        printf("Has succeeded: %d\n", succeeded);
        HMODULE hFluid = GetModuleHandle("libfluidsynth-3.dll");
        if (hFluid) {
            char path[MAX_PATH];
            GetModuleFileName(hFluid, path, MAX_PATH);
            printf("FluidSynth DLL successfully loaded from: %s\n", path);
        } else printf("FluidSynth DLL is not loaded yet or failed to locate.\n");
    }
#endif

int dllsSetup() {
#ifdef IS_WINDOWS    
    check_dll();
#endif

    return 0;
}