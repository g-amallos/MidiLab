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
        free(track->notes);
        track->notes = NULL;
    }
    track->numElements = 0;
    track->capacity = 0;
    track->keyMax = 0;
    track->keyMin = 0;
    track->timestampStart = 0;
    track->timestampEnd = 0;
}

static void _trackVectorUpdateValues(Track track) {
    if (!track) return;
    if (!(track->notes) || !(track->numElements)) {
        track->keyMin=0;
        track->keyMax=0;
        track->timestampStart=0;
        track->timestampEnd=0;
        return;
    }

    track->timestampStart = (track->notes)[0]->timestamp;
    uint8_t keyMin=128, keyMax=0;
    uint32_t timestampEnd=0, idx=0, num=track->numElements;
    for (; idx<num; idx++) {
        Note nt = (track->notes)[idx];
        if (nt->timestamp+nt->duration>timestampEnd) timestampEnd=nt->timestamp+nt->duration;
        if (nt->key>keyMax) keyMax=nt->key;
        if (nt->key<keyMin) keyMin=nt->key;
    }

    track->keyMax = keyMax;
    track->keyMin = keyMin;
    track->timestampEnd = timestampEnd;
}

void tracksUpdateAllValues() {
    if (!globalProject) return;
    uint16_t n=globalProject->tracksNum;
    for (uint16_t i=0; i<n; i++) {
        _trackVectorUpdateValues(globalProject->tracks+i);
    }
}

static void _trackVectorUpdateValuesOnNoteAddition(Track track, Note note) {
    if (!track || !(track->notes)) return;
    if (!(track->numElements)) {
        track->keyMin = note->key;
        track->keyMax = note->key;
        track->timestampStart = note->timestamp;
        track->timestampEnd = note->timestamp+note->duration;
    } else {
        if (track->keyMin>note->key) track->keyMin=note->key;
        if (track->keyMax<note->key) track->keyMax=note->key;
        if (track->timestampStart>note->timestamp) track->timestampStart=note->timestamp;
        if (track->timestampEnd<note->timestamp+note->duration) track->timestampEnd=note->timestamp+note->duration;
    }
}

static void _trackVectorUpdateValuesOnNoteRemoval(Track track, Note note) {
    if (!track) return;
    if (!(track->numElements) || note->key<=track->keyMin || note->key>=track->keyMax || note->timestamp<=track->timestampStart || note->timestamp+note->duration>=track->timestampEnd) _trackVectorUpdateValues(track);
}

int trackGetMinKey(Track track) {
    if (!track || !(track->numElements)) return -1;
    return track->keyMin;
}

int trackGetMaxKey(Track track) {
    if (!track || !(track->numElements)) return -1;
    return track->keyMax;
}

uint32_t trackGetTimestampEnd(Track track) {
    if (!track || !(track->numElements)) return 0;
    return track->timestampEnd;
}

void trackLoadTmpTrack(Track dest, Track src) {
    if (!dest || !src) return;
    freeTrackContents(dest);
    *dest = *src;
    _trackVectorUpdateValues(dest);
}

uint16_t tracksGetMaxTracks() {
    return 128;
}

int tracksCanCreateNew() {
    if (!globalProject) return 0;
    return (globalProject->tracksNum<tracksGetMaxTracks());
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

    //globalStateHandler->selectedTrack = (globalProject->tracksNum)++;
    Track ret = globalProject->tracks+(globalProject->tracksNum)++;

    ret->channel = 0;
    ret->program = 0;
    ret->sustain = 0;


    ret->velocity = 0.75;
    ret->panning = 0.5;

    ret->keyMax = 0;
    ret->keyMin = 0;
    ret->timestampStart = 0;
    ret->timestampEnd = 0;

    ret->capacity = 32;
    ret->numElements = 0;
    ret->notes = calloc((ret->capacity), sizeof(Note));     // Check if failed??

    ret->title = malloc(11*sizeof(char));
    if (ret->title) strncpy(ret->title, "New Track", 11);


    globalHandlerSelectTrack(globalProject->tracksNum-1);
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

int trackMoveToIndex(Track track, int idx) {
    if (!track || idx<0 || idx>=globalProject->tracksNum) return -1;
    if (track-globalProject->tracks==idx) return idx;

    int old=track-globalProject->tracks, new=idx;
    struct track_data strack = *track;
    if (old<new) {
        memmove(track, track+1, (new-old)*sizeof(struct track_data));
        (globalProject->tracks)[new] = strack;
    } else if (old>new) {
        memmove(globalProject->tracks+new+1, globalProject->tracks+new, (old-new)*sizeof(struct track_data));
        (globalProject->tracks)[new] = strack;
    }
    globalHandlerUpdateAllTracks();
    projectUpdateStateSomethingChanged();
    return new;
}

int trackCanSafelyReplaceContents(Track track) {
    if (!track) return 0;
    if (globalProject->saveState.state==S_STATE_SAVED && globalProject->saveState.filepath) return 1;
    return track->numElements==0;
}



void trackSortNotes(Track track);

static int _initTrackVector(Track track) {
    if (!track) return 1;
    if (!(track->notes)) {
        uint32_t cap = 32;
        track->notes = calloc(cap, sizeof(Note));
        track->capacity = cap*(!!(track->notes));
        track->numElements = 0;
    }
    return !(track->notes);
}

static int _trackVectorFindWhereNoteShouldBe(Track track, Note note) {
    if (!track || !(track->notes) || !note) return -1;
    uint32_t left=0, right=track->numElements;
    uint32_t target=note->timestamp;
    while (right>left) {
        uint32_t center = left+((right-left)>>1);
        if ((track->notes)[center]==note) return center;
        if ((track->notes)[center]->timestamp<target) left=center+1;
        else right=center;
    }
    return right;
}

static int _trackVectorFindNoteLinearlyAfter(Track track, Note note, int idx) {
    if (idx<0) return idx;
    uint32_t target=note->timestamp, nidx=(uint32_t)idx;
    while (nidx<track->numElements && note!=(track->notes)[nidx] && target<=(track->notes)[nidx]->timestamp) nidx++;
    if (nidx>=track->numElements || note!=(track->notes)[nidx]) return -1;
    else return (int)nidx;
}

static int _trackVectorFind(Track track, Note note) {
    int idx = _trackVectorFindWhereNoteShouldBe(track, note);
    return _trackVectorFindNoteLinearlyAfter(track, note, idx);
}

static int _trackVectorDuplicateCapacity(Track track) {
    uint32_t oldCap = track->capacity;
    uint32_t newCap = (oldCap<<1);

    Note* arr = realloc(track->notes, newCap*sizeof(Note));
    if (!arr) return 1;

    track->capacity = newCap;
    track->notes = arr;
    return 0;
}

static int _trackVectorHalveCapacity(Track track) {
    uint32_t oldCap = track->capacity;
    uint32_t newCap = (oldCap>>1);
    if (newCap<32) newCap=32;

    Note* arr = realloc(track->notes, newCap*sizeof(Note));
    if (!arr) return 1;

    track->capacity = newCap;
    track->notes = arr;
    return 0;
}

static void _trackVectorAdd(Track track, Note note) {
    if (_initTrackVector(track) || !note) return;
    int idx = _trackVectorFindWhereNoteShouldBe(track, note);
    int existIdx = _trackVectorFindNoteLinearlyAfter(track, note, idx);

    if (existIdx>=0) return;
    if (track->capacity<=track->numElements && _trackVectorDuplicateCapacity(track)) return;
    memmove(track->notes+idx+1, track->notes+idx, (track->numElements-idx)*sizeof(Note));
    (track->notes)[idx]=note;
    _trackVectorUpdateValuesOnNoteAddition(track, note);
    (track->numElements)++;
}

static int _trackVectorRemove(Track track, Note note) {
    if (_initTrackVector(track) || !note) return 1;
    int idx=_trackVectorFind(track, note), num=track->numElements;
    if (idx<0) return 1;
    (track->notes)[idx]=NULL;
    if (num-idx-1) memmove(track->notes+idx, track->notes+idx+1, (num-idx-1)*sizeof(Note));
    if (((track->capacity)>>2)>(--(track->numElements))) _trackVectorHalveCapacity(track);
    _trackVectorUpdateValuesOnNoteRemoval(track, note);
    return 0;
}


Note trackCreateNoteInTrack(Track track, uint8_t note, uint8_t velocity, uint32_t timestamp, uint32_t duration) {
    if (!track || note>127 || velocity>127) return NULL;

    Note mnote = malloc(sizeof(struct note_data));
    if (!mnote) return NULL;  // Malloc failed

    mnote->key = note;
    mnote->velocity = velocity;
    mnote->timestamp = timestamp;
    mnote->duration = duration;
    mnote->channel = track->channel;

    _trackVectorAdd(track, mnote);              // Improved from O(n*log(n)) (or depending on the implementation of qsort from O(n^2) worst-case) to O(n) worst case
    projectUpdateStateSomethingChanged();
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
    _trackVectorUpdateValues(track);
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
    if (!_trackVectorRemove(track, note)) free(note);
    projectUpdateStateSomethingChanged();
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