#include <stdlib.h>
#include "backend_internal.h"
#include <string.h>




void freeTrackNotes(Track track) {
    if (!track || !(track->notes)) return;

    for (uint32_t i=0; i<track->numElements; i++) if (track->notes[i]) free(track->notes[i]);
}

void freeTrackContents(Track track) {   // Doesn't free self
    if (!track) return;
    if (track->title) {
        free(track->title);
        track->title = NULL;
    }
    
    if (track->notes) {
        freeTrackNotes(track);
        track->numElements = 0;
        free(track->notes);
        track->notes = NULL;
    }

    track->capacity = 0;
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
    ret->sustain = 0;


    ret->velocity = 0.75;
    ret->panning = 0.5;

    ret->capacity = 32;
    ret->numElements = 0;
    ret->notes = calloc((ret->capacity), sizeof(Note));     // Check if failed??

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
    if (!track || velocity<0 || velocity>1) return;
    track->velocity=velocity;
}

float trackGetPanning(Track track) {
    if (!track) return 0;
    return track->panning;
}

void trackSetPanning(Track track, float panning) {
    if (!track || panning<0 || panning>1) return;
    track->panning=panning;
}

int trackGetSustain(Track track) {
    if (!track) return 0;
    return (int)(track->sustain);
}

int trackGetProgram(Track track) {
    if (!track) return 0;
    return track->program;
}

void trackSetProgram(Track track, uint8_t program) {    // 0-127: regular midi programs, 128: drums (channel 9)
    if (!track || program>128) return;
    track->program = program;    
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
        if (globalStateHandler) globalStateHandler->keys.type = T_KEYBOARD_NONE;
    }
    
}



void trackSortNotes(Track track);


void trackCreateNoteInTrack(Track track, uint8_t note, uint8_t velocity, uint32_t timestamp, uint32_t duration) {   // Not entirely done yet
    if (!track || note>127 || velocity>127) return;

    if (!(track->notes)) {
        track->capacity = 32;
        track->numElements = 0;
        track->notes = calloc((track->capacity), sizeof(Note));
    }


    if (track->numElements >= track->capacity) {
        uint32_t tcap = (track->numElements << 1);
        Note* tnotes = realloc(track->notes, tcap*sizeof(Note));
        if (!tnotes) return;    // Reallocation failed
        track->capacity = tcap;
        track->notes = tnotes;
    }

    Note mnote = malloc(sizeof(struct note_data));
    if (!mnote) return;  // Malloc failed

    mnote->key = note;
    mnote->velocity = velocity;
    mnote->timestamp = timestamp;
    mnote->duration = duration;
    mnote->channel = track->channel;

    // Should add the fields ftimestamp and fduration


    track->notes[(track->numElements)++] = mnote;

    trackSortNotes(track);
}




int trackNoteCompare(const void* a, const void* b) {
    if (!a && !b) return 0;
    if (!a || !b) return a?-1:1;
    float f = ((Note)b)->timestamp-((Note)a)->timestamp;
    return (f>0)?1:((f<0)?-1:0);
}

void trackSortNotes(Track track) {
    if (!track || !(track->notes) || !(track->numElements)) return;

    int items = track->numElements;
    if (!items) return;

    qsort(track->notes, items, sizeof(Note), trackNoteCompare);
    
}


void trackUpdateNotesFfields(Track track, uint16_t tempo) {
    if (!track || tempo<30 || tempo>2000) return;


}