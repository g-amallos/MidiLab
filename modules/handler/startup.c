#include <raylib.h>
#include <utils.h>
#include <interface.h>
#include <images.h>
#include <colors.h>


void updateWindowProjectTitle();


struct startup_data {
    int shouldRenderOverlay;
    int awaitingType;
    int allowActualRender;
    int allowUserInput;
    double extraTimeAfterInit;
    double opacityDuration;
    double firstRenderT;
    double projectInitT;
    double currentT;
};

static struct startup_data startup = {.shouldRenderOverlay=1, .awaitingType=0, .allowActualRender=0, .allowUserInput=0, .extraTimeAfterInit=1.5, .opacityDuration=0.5, .firstRenderT=0, .projectInitT=0, .currentT=0};


static void _startupScreenRender(Vector2 screenSize, float opacity) {
    float fa = trigInterpolation(0, 255, opacity);
    unsigned char a = (unsigned char)(fa);

    DrawRectangleGradientH(0,0,(int)(screenSize.x),(int)(screenSize.y), (Color){13,24,28,a}, (Color){25,17,35,a});

    Vector2 iconDims = iconGetDimensions(T_ICON_MIDILAB_LOGO_1024);
     
    Rectangle target = rectangleScaleToFitInCenter(iconDims, (Rectangle){0,0,screenSize.x,screenSize.y});
    float scale = trigInterpolation(0.7, 0.9, 1-opacity);
    target = scaleRctangleFromCenter(target, scale);

    Vector2 center = (Vector2){screenSize.x*0.5, screenSize.y*0.5};
    Color c1=COLOR_TRACK_THEME_2, c2=COLOR_TRACK_THEME_4;
    c1.a=(unsigned char)(0.008*fa), c2.a=(unsigned char)(0.15*fa);
    DrawCircleGradient(center, floatMin(screenSize.x, screenSize.y)*0.5*scale, c2, c1);

    Color col = (Color){255,255,255,a};
    iconRerder(T_ICON_MIDILAB_LOGO_1024, target, col);

    //renderFPS();
}

static float _startupGetOpacity() {
    if (!(startup.shouldRenderOverlay)) return 0.0;
    
    if (startup.awaitingType==0 || startup.currentT<=startup.extraTimeAfterInit+startup.projectInitT) return 1.0;
    if (startup.currentT>=startup.extraTimeAfterInit+startup.projectInitT+startup.opacityDuration) return 0.0;
    return (startup.extraTimeAfterInit+startup.projectInitT+startup.opacityDuration-startup.currentT)/startup.opacityDuration;
}

static void _firstScreen(Vector2 screenSize) {
    BeginDrawing();
    
    _startupScreenRender(screenSize, 1.0);

    SwapScreenBuffer();
    PollInputEvents();
}

int startupScreenRenderIfNeeded(Vector2 screenSize) {
    if (!(startup.shouldRenderOverlay)) return 0;
    _startupScreenRender(screenSize, _startupGetOpacity());
    return 0;
}

int startupScreenRender(Vector2 screenSize) {
    BeginDrawing();
        _startupScreenRender(screenSize, _startupGetOpacity());
    EndDrawing();
    
    return 0;
}

int startupSetup(Vector2 screenSize) {
    iconsPreInit();

    _firstScreen(screenSize);
    startup.firstRenderT = GetTime();
    startup.currentT = startup.firstRenderT;
    return 0;
}

int startupScreenUpdate() {
    if (!(startup.shouldRenderOverlay)) return 0;

    double curT = GetTime();
    
    startup.currentT = curT;
    if (startup.awaitingType==0) {
        startup.awaitingType=1;
        startup.projectInitT=curT;
    } else if (startup.awaitingType==1) {
        if (curT>startup.extraTimeAfterInit+startup.projectInitT) {
            startup.allowActualRender = 1;
            if (curT<startup.extraTimeAfterInit+startup.opacityDuration+startup.projectInitT) {
                startup.allowUserInput = 0;
            } else {
                startup.allowUserInput = 1;
                startup.shouldRenderOverlay = 0;
                updateWindowProjectTitle();
            }
        }
    }

    return 0;
}



int startupRenderAllowed() {
    return startup.allowActualRender;
}

int startupInputAllowed() {
    return startup.allowUserInput;
}