#include <stdlib.h>
#include "backend_internal.h"
#include <backend.h>
#include <string.h>
#include <stdio.h>


void _destroyNote(void* note) {
    free(note);
}

int _compareNotes(const void* a, const void* b) {   // Passing Note*
    if (!a && !b) return 0;
    if (!a || !b) return a?-1:1;
    
    const Note na = *(const Note*)a;
    const Note nb = *(const Note*)b;

    if (na->timestamp>nb->timestamp) return 1;
    if (na->timestamp<nb->timestamp) return -1;
    if (na->key>nb->key) return 1;
    if (na->key<nb->key) return -1;
    if (na->duration>nb->duration) return 1;
    if (na->duration<nb->duration) return -1;
    return 0;
}

void freeTrackContents(Track track) {   // Doesn't free self
    if (!track) return;
    if (track->title) {
        free(track->title);
        track->title = NULL;
    }
    
    if (track->notes) {
        sortedVectorDestroy(track->notes);
        track->notes = NULL;
    }
    track->keyMax = 0;
    track->keyMin = 0;
    track->timestampStart = 0;
    track->timestampEnd = 0;
}

static void _trackVectorUpdateValues(Track track) {
    if (!track) return;
    uint32_t num=sortedVectorGetSize(track->notes);
    const Note* arr=(const Note*)sortedVectorGetElements(track->notes);
    if (!num || !arr) {
        track->keyMin=0;
        track->keyMax=0;
        track->timestampStart=0;
        track->timestampEnd=0;
        return;
    }

    track->timestampStart = (arr[0])->timestamp;
    uint8_t keyMin=128, keyMax=0;
    uint32_t timestampEnd=0, idx=0;
    for (; idx<num; idx++) {
        Note nt = arr[idx];
        if (nt->timestamp+nt->duration>timestampEnd) timestampEnd=nt->timestamp+nt->duration;
        if (nt->key>keyMax) keyMax=nt->key;
        if (nt->key<keyMin) keyMin=nt->key;
    }

    track->keyMax = keyMax;
    track->keyMin = keyMin;
    track->timestampEnd = timestampEnd;
}

void trackRecalculateValues(Track track) {
    _trackVectorUpdateValues(track);
}

void tracksUpdateAllValues() {
    if (!globalProject) return;
    uint16_t n=globalProject->tracksNum;
    for (uint16_t i=0; i<n; i++) {
        _trackVectorUpdateValues(globalProject->tracks+i);
    }
}

/*
uint32_t trackGetDurationTimestamp(Track track) {
    if (!track) return 0;
    return track->timestampEnd;
}*/

static void _trackVectorUpdateValuesOnNoteAddition(Track track, Note note) {
    if (!track) return;
    uint32_t num = sortedVectorGetSize(track->notes);
    if (!num) {
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


int trackGetMinKey(Track track) {
    if (!track || !sortedVectorGetSize(track->notes)) return -1;
    return track->keyMin;
}

int trackGetMaxKey(Track track) {
    if (!track || !sortedVectorGetSize(track->notes)) return -1;
    return track->keyMax;
}

uint32_t trackGetTimestampEnd(Track track) {
    if (!track || !sortedVectorGetSize(track->notes)) return 0;
    return track->timestampEnd;
}

void trackLoadTmpTrack(Track dest, Track src) {
    if (!dest || !src) return;
    freeTrackContents(dest);
    *dest = *src;
    _trackVectorUpdateValues(dest);     // Continue from here!!
}

uint16_t tracksGetMaxTracks() {
    return 128;
}

uint16_t trackGetMaxTitleLength() {
    return 22;
}

int tracksCanCreateNew() {
    if (!globalProject) return 0;
    return (globalProject->tracksNum<tracksGetMaxTracks());
}


Track trackCreateNew() {
    if (!tracksCanCreateNew()) return NULL;

    struct track_data tmp = {.channel=0, .program=0, .sustain=0, .velocity=0.75, .panning=0.5, .keyMax=0, .keyMin=0, .timestampStart=0, .timestampEnd=0, .title=NULL, .notes=NULL};
    tmp.title = malloc(11*sizeof(char));
    if (tmp.title) strncpy(tmp.title, "New Track", 11);

    tmp.notes = sortedVectorCreate(_compareNotes, _destroyNote);
    if (!(tmp.title) || !(tmp.notes)) {
        if (tmp.title) free(tmp.title);
        if (tmp.notes) sortedVectorDestroy(tmp.notes);
        return NULL;
    }

    if (globalProject->tracks) {
        Track rlc = realloc(globalProject->tracks, (globalProject->tracksNum+1)*sizeof(struct track_data));
        if (!rlc) {
            if (tmp.title) free(tmp.title);
            if (tmp.notes) sortedVectorDestroy(tmp.notes);
            return NULL;
        }
        globalProject->tracks = rlc;
    } else {
        globalProject->tracks = malloc(sizeof(struct track_data));
        if (!(globalProject->tracks)) {
            if (tmp.title) free(tmp.title);
            if (tmp.notes) sortedVectorDestroy(tmp.notes);        
            return NULL;
        }
    }

    Track ret = globalProject->tracks+(globalProject->tracksNum)++;
    *ret = tmp;

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

    if (idx<globalProject->tracksNum-1) memmove(globalProject->tracks+idx, globalProject->tracks+idx+1, sizeof(struct track_data)*(globalProject->tracksNum-idx-1));

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
    return sortedVectorGetSize(track->notes)==0;
}



void trackSortNotes(Track track);



Note trackCreateNoteInTrack(Track track, uint8_t note, uint8_t velocity, uint32_t timestamp, uint32_t duration) {
    if (!track || note>127 || velocity>127) return NULL;

    Note mnote = malloc(sizeof(struct note_data));
    if (!mnote) return NULL;  // Malloc failed

    mnote->key = note;
    mnote->velocity = velocity;
    mnote->timestamp = timestamp;
    mnote->duration = duration;
    mnote->channel = track->channel;

    _trackVectorUpdateValuesOnNoteAddition(track, mnote);
    sortedVectorAdd(track->notes, mnote);              // Improved from O(n*log(n)) (or depending on the implementation of qsort from O(n^2) worst-case) to O(n) worst case
    projectUpdateStateSomethingChanged();
    return mnote;
}


void trackVectorAddNewNotes(Track track, SortedVector svec) {
    if (!track || !svec) return;
    sortedVectorMerge(track->notes, svec);
    _trackVectorUpdateValues(track);
}

void trackDeleteNoteInTrackByIdx(Track track, uint32_t idx) {
    if (!track) return;
    sortedVectorDeleteElementAt(track->notes, idx);
    _trackVectorUpdateValues(track);
    projectUpdateStateSomethingChanged();
}


void trackDeleteNoteInTrack(Track track, Note note) {
    if (!track || !note) return;
    sortedVectorDeleteElement(track->notes, note);
    projectUpdateStateSomethingChanged();
}





void trackSortNotes(Track track) {
    if (!track || !(track->notes)) return;
    sortedVectorSort(track->notes);
}

void sortNoteBuffer(Note* buffer, uint64_t size) {
    if (!buffer || !size) return;
    qsort(buffer, size, sizeof(Note), _compareNotes);
}



void trackUpdateAllNoteData(Track track) {
    if (!track || !(globalProject) || !(track->notes)) return;

    uint8_t trackIdx = track-globalProject->tracks;
    uint32_t n = sortedVectorGetSize(track->notes);
    Note* arr = (Note*)sortedVectorGetElements(track->notes);
    for (uint32_t i=0; i<n; i++) {
        Note nt = arr[i];
        nt->track = trackIdx;
        nt->ftimestamp = globalHandlerTimestampToSeconds(nt->timestamp);
        nt->fduration = globalHandlerTimestampToSeconds(nt->duration);
    }
}

uint32_t trackPiecesInBeat() {
    return (1<<14);
}


uint32_t trackGetNumOfNotes(Track track) {
    if (!track) return 0;
    return sortedVectorGetSize(track->notes);
}

Note* trackGetNotes(Track track) {
    if (!track) return 0;
    return (Note*)sortedVectorGetElements(track->notes);
}

void tracksDebugPrint(Track track) {
    if (!track) return;
    uint32_t n=sortedVectorGetSize(track->notes);
    const Note* arr = (const Note*)sortedVectorGetElements(track->notes);
    printf("Track #%p: %u Notes\n", (void*)track, n);
    for (uint32_t i=0; i<n; i++) {
        printf("- %3u: %p\n", i, (void*)arr[i]);
    }
}