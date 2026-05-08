#include <stdlib.h>
#include "backend_internal.h"
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



Track trackCreateNew() {
    if (!globalProject) return NULL;

    if (globalProject->tracks) {
        Track rlc = realloc(globalProject->tracks, (globalProject->tracksNum+1)*sizeof(struct track_data));
        if (!rlc) return NULL;
        globalProject->tracks = rlc;
    } else {
        globalProject->tracks = malloc(sizeof(struct track_data));
        if (!(globalProject->tracks)) return NULL;
    }

    Track ret = globalProject->tracks+(globalProject->tracksNum)++;

    ret->channel = 0;
    ret->program = 0;
    ret->velocity = 1;
    ret->internalElements = 32;
    ret->externalElements = 0;
    ret->notes = calloc((ret->internalElements), sizeof(struct note_data));     // Check if failed??

    ret->title = malloc(11*sizeof(char));
    if (ret->title) strncpy(ret->title, "New Track", 11);

    return ret;
}

Track trackGetAtIdx(int idx) {
    if (!globalProject || idx<0 || idx>=globalProject->tracksNum) return NULL;
    return globalProject->tracks+idx;
}

const char* trackGetTitle(Track track) {
    if (!track) return NULL;
    return track->title;
}

void trackSetTitle(Track track, const char* title) {
    if (!track) return;
    int len = strlen(title);
    char* new = malloc((len+1)*sizeof(char));
    if (!new) return;
    if (track->title) free(track->title);
    track->title = new;
    strncpy(new, title, len+1);
}

float trackGetVelocity(Track track) {
    if (!track) return 0;
    return track->velocity;
}

void trackSetVelocity(Track track, float velocity) {
    if (!track) return;
    track->velocity=velocity;
}

int trackGetProgram(Track track) {
    if (!track) return 0;
    return track->program;
}

void trackDeleteAtIdx(int idx) {
    if (!globalProject || idx<0 || idx>=globalProject->tracksNum) return;

    Track old = &(globalProject->tracks)[idx];
    freeTrackContents(old);

    
    for (int i=idx; i<globalProject->tracksNum-1; i++) {
        (globalProject->tracks)[i] = (globalProject->tracks)[i+1];
    }

    if (globalProject->tracksNum>1) globalProject->tracks = realloc(globalProject->tracks, (--(globalProject->tracksNum))*sizeof(struct track_data));
    else {
        free(globalProject->tracks);
        globalProject->tracks=NULL;
        globalProject->tracksNum = 0;
    }
    
}