#include <raylib.h>
#include <utils.h>


struct inactivityHandler {
    double currentT;
    double lastActiveT;
    float inactiveForT;
    float opacity;
};

struct inactivityHandler GlobalInactivityHandler = {.currentT=0, .lastActiveT=0, .inactiveForT=0, .opacity=1};




void updateInactivityStruct() {
    double t = GetTime();
    GlobalInactivityHandler.currentT = t;
    if (IsWindowFocused()) GlobalInactivityHandler.lastActiveT = t;
    GlobalInactivityHandler.inactiveForT = GlobalInactivityHandler.currentT-GlobalInactivityHandler.lastActiveT;
    if (GlobalInactivityHandler.inactiveForT<=0) GlobalInactivityHandler.opacity = 0;
    else if (GlobalInactivityHandler.inactiveForT>=1) GlobalInactivityHandler.opacity = 1;
    else GlobalInactivityHandler.opacity = trigInterpolation(0, 1, GlobalInactivityHandler.inactiveForT);
}