#ifndef TILES_H
#define TILES_H



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

struct tiles_settings {
    int beatsInMeasure;
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



    struct duration totalDuration;
    struct tiles_info info;
    struct note_array notes;
};

struct note_column {
    uint8_t key;

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


#endif