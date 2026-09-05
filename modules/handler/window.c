#include <raylib.h>
#include <string.h>
#include <utils.h>
#include <handler.h>
#include <stdlib.h>
#include <tinyfiledialogs.h>
#include <backend.h>
#include <synth.h>


void updateWindowTitle(const char* title, int saved) {
    char* ttl = stringStrip(title);
    int length = strlen(ttl);

    if (!ttl || length==0) {
        free(ttl);
        SetWindowTitle("MidiLab");
    } else {
        const char* scndhlf = NULL;
        if (saved) scndhlf = " - MidiLab";
        else scndhlf = "* - MidiLab";
        char* out = concatenateStrings(ttl, scndhlf);
        free(ttl);
        SetWindowTitle(out);
        free(out);
    }
}


int windowShouldCloseDialog() {
    if (projectCanSafelyReplaceContents()) return 1;
    synthPanic();
    int result = tinyfd_messageBox("Warning", "Are you sure you want to close MidiLab?\nYour current project will be lost.", "yesno", "warning", 0);
    return result==1;    
}

int windowToggleFullscreenIfNecessary() {
    if (IsKeyPressed(KEY_F11)) {
        ToggleBorderlessWindowed();
        //ClearWindowState(FLAG_WINDOW_TOPMOST);
    }
    return 0;
}