#ifndef TILES_H
#define TILES_H

#include <raylib.h>
#include <ui.h>


struct duration {
    double seconds;
    struct {
        int beats;
        float remainder;
    } beats;
};

enum setting_theme {
    THEME_PROJECT=0,
    THEME_CLASSIC,
    THEME_CLEAN,
    THEME_TEST,

    THEME_END,
};

enum setting_aspect_ratio {
    ASPECT_RATIO_9_16=0,
    ASPECT_RATIO_3_4,
    ASPECT_RATIO_1_1,
    ASPECT_RATIO_4_3,
    ASPECT_RATIO_16_9,

    ASPECT_RATIO_END,
};

enum setting_bool {
    SETTING_OFF,
    SETTING_ON
};


enum setting_type {
    SETTING_BOOL=0,
    SETTING_INT,
    SETTING_FLOAT,

};

struct float_constraints {
    double min;
    double max;
};

struct track_tile_settings {
    Track track;
    int show;
    int colorMode;
    int totalNotes;

    Color color;
    Button base;
    Button showBtn;
    Button colorBtn;
    Slider sld[3];
};


struct tiles_settings {
    int beatsInMeasure;
    enum setting_aspect_ratio aspectRatio;
    enum setting_theme theme;
    enum setting_bool timeNotation;
    enum setting_bool countMeasures;
    enum setting_bool identifyChords;
    enum setting_bool showDrums;



    struct float_constraints delayConstraints;
    struct duration startDelay;
    struct duration endDelay;


    struct float_constraints visDurConstraints;
    struct duration visibleDuration;

    struct float_constraints keyHeightConstraints;
    double keyHeight;

    struct float_constraints spacingConstraints;
    double space;


    int totalTracks;
    struct track_tile_settings* tracks;


    struct duration totalDuration;
    struct tiles_info info;
    struct note_array notes;
};

struct note_column {
    uint8_t key;
    uint8_t isDown;
    float holdForce;
    Color color;

    float x;
    float w;
};

struct view_data {
    int totalKeys;
    int whiteKeys;
    float spacePerc;
    float whiteKeyPerc;
    float keyHeight;
    float blackKeyWidth;
    float blackKeyHeight;
    
    struct note_column* columns;
};




extern struct tiles_settings settings;
extern struct view_data view;
extern double beatDuration;
extern Rectangle previewDiv, previewRect;
extern float divsPixelRadius;


void tilesSetUpTargetRectangle(Rectangle rect);
void tilesSetUpColors(Color tilesBackground, Color disabledBackground, Color textOrHoverBackground, Color settingsBackground);
void _renderTilesToTargetRect();
double _tilesControlGetTime();
void tilesExportVideoClose();
void tilesExportVideoInit();



#endif