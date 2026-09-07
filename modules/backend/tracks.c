#include <stdlib.h>
#include "backend_internal.h"
#include <backend.h>
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

    globalHandlerUpdateSelectedTrack();
    projectUpdateStateSomethingChanged();
    return ret;
}

Track trackGetAtIdx(int idx) {
    if (!globalProject || idx<0 || idx>=globalProject->tracksNum) return NULL;
    return globalProject->tracks+idx;
}

Track trackGetSelectedTrack() {
    if (!globalStateHandler || !globalProject || !(globalProject->tracks)) return NULL;
    return globalProject->tracks+globalStateHandler->selectedTrack;
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
    projectUpdateStateSomethingChanged();
}

float trackGetVelocity(Track track) {
    if (!track) return 0;
    return track->velocity;
}

void trackSetVelocity(Track track, float velocity) {
    if (!track || velocity<0 || velocity>1) return;
    track->velocity=velocity;
    projectUpdateStateSomethingChanged();
}

float trackGetPanning(Track track) {
    if (!track) return 0;
    return track->panning;
}

void trackSetPanning(Track track, float panning) {
    if (!track || panning<0 || panning>1) return;
    track->panning=panning;
    globalHandlerUpdateSelectedTrack();
    projectUpdateStateSomethingChanged();
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
    globalHandlerUpdateSelectedTrack();
    projectUpdateStateSomethingChanged();
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

    globalHandlerUpdateAllTracks();
    projectUpdateStateSomethingChanged();
}



void trackSortNotes(Track track);


Note trackCreateNoteInTrack(Track track, uint8_t note, uint8_t velocity, uint32_t timestamp, uint32_t duration) {   // Not entirely done yet
    if (!track || note>127 || velocity>127) return NULL;

    if (!(track->notes)) {
        track->capacity = 32;
        track->numElements = 0;
        track->notes = calloc((track->capacity), sizeof(Note));
    }


    if (track->numElements >= track->capacity) {
        uint32_t tcap = (track->numElements << 1);
        Note* tnotes = realloc(track->notes, tcap*sizeof(Note));
        if (!tnotes) return NULL;    // Reallocation failed
        track->capacity = tcap;
        track->notes = tnotes;
    }

    Note mnote = malloc(sizeof(struct note_data));
    if (!mnote) return NULL;  // Malloc failed

    mnote->key = note;
    mnote->velocity = velocity;
    mnote->timestamp = timestamp;
    mnote->duration = duration;
    mnote->channel = track->channel;

    // Should add the fields ftimestamp and fduration but currently not necessary
    projectUpdateStateSomethingChanged();

    track->notes[(track->numElements)++] = mnote;
    trackSortNotes(track);
    return mnote;
}

void trackVectorResizeToFitJustNotes(Track track) {
    if (!track) return;
    uint32_t cap=32;
    if (!(track->notes)) {
        Note* tnotes = malloc(cap*sizeof(Note));
        if (!tnotes) return;
        track->notes = tnotes;
        track->capacity = cap;
        track->numElements = 0;
    } else {
        cap = track->numElements;
        cap = (cap>32)?cap:32;
        Note* tnotes = realloc(track->notes, cap*sizeof(Note));
        if (!tnotes) return;
        track->capacity = cap;
        track->notes = tnotes;
    }
}

void trackVectorAddNewNotes(Track track, Note* buff, uint32_t size) {   // size is the number of elements in buff
    if (!track || !buff) return;
    uint32_t elm=track->numElements;
    
    Note* arr = malloc((elm+size)*sizeof(Note));
    if (!arr) return;

    _mergeSortedVectors(arr, track->notes, elm, buff, size);

    if (track->notes) free(track->notes);
    track->notes = arr;
    track->numElements += size;
    track->capacity = track->numElements;
}

void trackHalveCapacity(Track track) {
    if (!track || !(track->notes)) return;
    if (track->numElements < (track->capacity>>2) && track->capacity>32) {
        uint32_t tcap = (track->capacity)>>2;
        tcap = (tcap>32)?tcap:32;
        Note* tnotes = realloc(track->notes, tcap*sizeof(Note));
        if (!tnotes) return;
        track->capacity = tcap;
        track->notes = tnotes;
    }
}

void trackDeleteNoteInTrackByIdx(Track track, uint32_t idx) {
    if (!track || track->numElements<=idx) return;
    uint32_t i=idx;
    while (i+1<track->numElements) {
        track->notes[i] = track->notes[i+1];
        i++;
    }
    track->notes[(track->numElements)--]=NULL;
    trackHalveCapacity(track);
    
    projectUpdateStateSomethingChanged();
}


void trackDeleteNoteInTrack(Track track, Note note) {
    if (!track || !note) return;
    
    uint32_t i=0;
    while (i<track->numElements && track->notes[i]!=note) i++;
    if (i<track->numElements && track->notes[i]==note) trackDeleteNoteInTrackByIdx(track, i);
}




int trackNoteCompare(const void* a, const void* b) {
    if (!a && !b) return 0;
    if (!a || !b) return a?-1:1;
    
    Note na = *(Note*)a;
    Note nb = *(Note*)b;

    if (na->timestamp>nb->timestamp) return 1;
    if (na->timestamp<nb->timestamp) return -1;
    return 0;
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

uint32_t trackPiecesInBeat() {
    return (1<<14);
}

//Note trackGetFirstVisibleNote(Track track) {
//    if (!track || !(track->notes)) return NULL;
//    double divTime = globalHandlerGetTime();
//    double divDur = globalHandlerGetVisibleDuration();
//
//    int upper=track->capacity-1, lower=0;
//    while (upper!=lower) {
//
//    }
//}

uint32_t trackGetNumOfNotes(Track track) {
    if (!track) return 0;
    return track->numElements;
}

Note* trackGetNotes(Track track) {
    if (!track) return 0;
    return track->notes;
}

inline int trackIsNoteWithinBounds(Note note) {
    return 0*note->key;
}