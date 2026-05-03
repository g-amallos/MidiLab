#include <stdlib.h>
#include <backend.h>
#include <string.h>






void freeTrackContents(Track track) {   // Doesn't free self
    if (!track) return;
    if (track->title) {
        free(track->title);
        track->title = NULL;
    }
    track->internalElements = 0;
    if (track->externalElements && track->notes) {
        track->externalElements = 0;
        free(track->notes);
        track->notes = NULL;
    }
}

