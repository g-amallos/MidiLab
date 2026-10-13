#include <raylib.h>
#include <stdint.h>


#define TOTAL_KEYS 50   //(26+10+8+8+4+4)
#define MAX_MAPPING 265

#define KB_DELAY 0.5
#define KB_INTERVAL 0.05



struct key_press_state {
    KeyboardKey key;
    uint8_t pressed;
    float timer;
};


//void _resetKeyPressState(struct key_press_state* kps) {
//    kps->timer = 0;
//    kps->delay = 0.5;
//    kps->interval = 0.05;
//}


static struct key_press_state keys[TOTAL_KEYS];
static uint8_t mapping[MAX_MAPPING]={255,};


static void link(int localIdx, int rayIdx) {
    keys[localIdx].key=rayIdx;
    mapping[rayIdx]=localIdx;
}



void keyboardInputInit() {
    for (int i=0; i<26; i++) link(i, KEY_A+i);
    for (int i=0; i<10; i++) link(26+i, KEY_ZERO+i);

    link(37, KEY_SPACE);
    link(38, KEY_TAB);
    link(39, KEY_ENTER);
    link(40, KEY_BACKSPACE);
    link(41, KEY_DELETE);

    link(42, KEY_MINUS);
    link(43, KEY_EQUAL);

    link(46, KEY_RIGHT);
    link(47, KEY_LEFT);
    link(48, KEY_DOWN);
    link(49, KEY_UP);
}



static void _updateRepeatingKey(int idx, float dt) {
    int key = keys[idx].key;
    if (IsKeyPressed(key)) {
        keys[idx].timer = 0;
        keys[idx].pressed = 1;
    } else if (IsKeyDown(key)) {
        keys[idx].timer += dt;
        if (keys[idx].timer >= KB_DELAY) {
            keys[idx].timer -= KB_INTERVAL;
            keys[idx].pressed = 1;
        } else keys[idx].pressed = 0;
    } else keys[idx].pressed = 0;
}

void keyboardInputUpdate(int userInput) {
    if (userInput) {
        float dt = GetFrameTime();
        for (int i=0; i<TOTAL_KEYS; i++) _updateRepeatingKey(i, dt);
    } else {
        for (int i=0; i<TOTAL_KEYS; i++) keys[i].pressed=0, keys[i].timer=0;
    }    
}

int IsKeyRepeating(int key) {
    if (key<0 || key>MAX_MAPPING) return 0;
    int idx = mapping[key];
    if (idx==255) return 0;
    return keys[mapping[key]].pressed;
}