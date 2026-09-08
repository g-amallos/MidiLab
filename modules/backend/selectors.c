#include "backend_internal.h"
#include <utils.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>



static void _freeNoteSelector() {
    if (globalStateHandler->selector.primary.notes) free(globalStateHandler->selector.primary.notes);
    if (globalStateHandler->selector.secondary.notes) free(globalStateHandler->selector.secondary.notes);

    struct notes_selection tmpSel = {
        .keyMin = 255,
        .keyMax = 0,
        .clickHold = 0,
        .rollRect = {
            .topLeft = {.fkey=-1, .timestamp=0},
            .bottomRight = {.fkey=-1, .timestamp=0},
        },
        .noteReference = {NULL,},
        .timestampStart = 0,
        .timestampEnd = 0,
        .capacity = 0,
        .notesNum = 0,
        .notes = NULL
    };

    globalStateHandler->selector.primary = tmpSel;
    globalStateHandler->selector.secondary = tmpSel;
}

void globalHandlerClearNotesSelected() {
    if (!globalStateHandler) return;
    _freeNoteSelector();
}

static void _selectorVectorReset(struct notes_selection* selector) {
    if (!selector) return;
    if (selector->notes) free(selector->notes);
    *selector = (struct notes_selection){
        .keyMin = 255,
        .keyMax = 0,
        .clickHold = 0,
        .rollRect = {
            .topLeft = {.fkey=-1, .timestamp=0},
            .bottomRight = {.fkey=-1, .timestamp=0},
        },
        .noteReference = {NULL,},
        .timestampStart = 0,
        .timestampEnd = 0,
        .capacity = 0,
        .notesNum = 0,
        .notes = NULL
    };
}

static void _freeNotes(Note* buff, uint32_t size) {
    if (!buff) return;
    for (uint32_t i=0; i<size; i++) {
        if (buff[i]) free(buff[i]);
        buff[i] = NULL;                 // Theoretically shouldn't be necessary, but anyways
    }
}

static void _selectorVectorDeepFree(struct notes_selection* selector) {
    if (!selector) return;
    _freeNotes(selector->notes, selector->notesNum);
    _selectorVectorReset(selector);
}

static int _initSelectorVector(struct notes_selection* selector) {
    if (!selector) return 1;
    if (!(selector->notes)) {
        uint32_t cap = 32;
        selector->notes = calloc(cap, sizeof(Note));
        selector->capacity = cap*(!!(selector->notes));
        selector->notesNum = 0;
    }
    return !(selector->notes);
}

static int _selectorVectorDuplicateCapacity(struct notes_selection* selector) {
    uint32_t oldCap = selector->capacity;
    uint32_t newCap = (oldCap<<1);

    Note* arr = realloc(selector->notes, newCap*sizeof(Note));
    if (!arr) return 1;

    selector->capacity = newCap;
    selector->notes = arr;
    return 0;
}

static int _selectorVectorHalveCapacity(struct notes_selection* selector) {
    uint32_t oldCap = selector->capacity;
    uint32_t newCap = (oldCap>>1);
    if (newCap<32) newCap=32;

    Note* arr = realloc(selector->notes, newCap*sizeof(Note));
    if (!arr) return 1;

    selector->capacity = newCap;
    selector->notes = arr;
    return 0;
}

static int _intCompareFunc(const void* a, const void* b) {
    if (!a && !b) return 0;
    if (!a || !b) return a?-1:1;
    int na=*(int*)a, nb = *(int*)b;
    if (na>nb) return 1;
    if (na<nb) return -1;
    return 0;
}

static int _selectorVectorNoteCompare(const void* a, const void* b) {
    if (!a && !b) return 0;
    if (!a || !b) return a?-1:1;
    
    Note na = *(Note*)a;
    Note nb = *(Note*)b;

    if (na->timestamp>nb->timestamp) return 1;
    if (na->timestamp<nb->timestamp) return -1;
    return 0;
}

/*static*/ uint32_t _tracksGetTimestampIdx(Track track, uint32_t timestamp) {
    if (!track || !(track->notes)) return 0;
    uint32_t left=0, right=track->numElements;
    while (right>left) {
        uint32_t center = left+((right-left)>>1);
        if ((track->notes)[center]->timestamp<timestamp) left=center+1;
        else right=center;
    }
    return right;
}

static int _tracksVectorFindWhereNoteShouldBe(Track track, Note note) {
    if (!track || !(track->notes) || !note) return -1;
    uint32_t left=0, right=track->numElements;
    uint32_t target=note->timestamp;
    while (right>left) {
        uint32_t center = left+((right-left)>>1);
        if ((track->notes)[center]==note) return center;
        if ((track->notes)[center]->timestamp<target) left=center+1;
        else right=center;
    }
    uint32_t nidx=(uint32_t)right;
    while (nidx<track->numElements && note!=(track->notes)[nidx] && target<=(track->notes)[nidx]->timestamp) nidx++;
    if (nidx>=track->numElements || note!=(track->notes)[nidx]) return -1;
    else return (int)nidx;
}

static int _selectorVectorFindWhereNoteShouldBe(struct notes_selection* selector, Note note) {
    if (!selector || !(selector->notes) || !note) return -1;
    uint32_t left=0, right=selector->notesNum;
    uint32_t target=note->timestamp;
    while (right>left) {
        uint32_t center = left+((right-left)>>1);
        if ((selector->notes)[center]==note) return center;
        if ((selector->notes)[center]->timestamp<target) left=center+1;
        else right=center;
    }
    return right;
}

static int _selectorVectorFindNoteLinearlyAfter(struct notes_selection* selector, Note note, int idx) {
    if (idx<0) return idx;
    uint32_t target=note->timestamp, nidx=(uint32_t)idx;
    while (nidx<selector->notesNum && note!=(selector->notes)[nidx] && target<=(selector->notes)[nidx]->timestamp) nidx++;
    if (nidx>=selector->notesNum || note!=(selector->notes)[nidx]) return -1;
    else return (int)nidx;
}

static int _selectorVectorFind(struct notes_selection* selector, Note note) {
    int idx = _selectorVectorFindWhereNoteShouldBe(selector, note);
    return _selectorVectorFindNoteLinearlyAfter(selector, note, idx);
}

/*static*/ void _selectorVectorSort(struct notes_selection* selector) {
    if (!selector || !(selector->notes) || !(selector->notesNum)) return;
    qsort(selector->notes, selector->notesNum, sizeof(Note), _selectorVectorNoteCompare);
}

static void _selectorVectorUpdateValuesOnNoteAddition(struct notes_selection* selector, Note note) {
    if (!selector || !(selector->notes)) return;
    if (!(selector->notesNum)) {
        selector->keyMin = note->key;
        selector->keyMax = note->key;
        selector->timestampStart = note->timestamp;
        selector->timestampEnd = note->timestamp+note->duration;
    } else {
        if (selector->keyMin>note->key) selector->keyMin=note->key;
        if (selector->keyMax<note->key) selector->keyMax=note->key;
        if (selector->timestampStart>note->timestamp) selector->timestampStart=note->timestamp;
        if (selector->timestampEnd<note->timestamp+note->duration) selector->timestampEnd=note->timestamp+note->duration;
    }
}

static void _selectorVectorUpdateValues(struct notes_selection* selector) {
    if (!selector) return;
    if (!(selector->notes) || !(selector->notesNum)) {
        selector->keyMin=0;
        selector->keyMax=0;
        selector->timestampStart=0;
        selector->timestampEnd=0;
        return;
    }

    selector->timestampStart = (selector->notes)[0]->timestamp;
    uint8_t keyMin=128, keyMax=0;
    uint32_t timestampEnd=0, idx=0, num=selector->notesNum;
    for (; idx<num; idx++) {
        Note nt = (selector->notes)[idx];
        if (nt->timestamp+nt->duration>timestampEnd) timestampEnd=nt->timestamp+nt->duration;
        if (nt->key>keyMax) keyMax=nt->key;
        if (nt->key<keyMin) keyMin=nt->key;
    }

    selector->keyMax = keyMax;
    selector->keyMin = keyMin;
    selector->timestampEnd = timestampEnd;
}

static void _selectorVectorUpdateValuesOnNoteRemoval(struct notes_selection* selector, Note note) {
    if (!selector) return;
    if (!(selector->notesNum) || note->key<=selector->keyMin || note->key>=selector->keyMax || note->timestamp<=selector->timestampStart || note->timestamp+note->duration>=selector->timestampEnd) _selectorVectorUpdateValues(selector);
}

static void _selectorVectorAddLast(struct notes_selection* selector, Note note) {
    if (_initSelectorVector(selector) || !note) return;
    int idx = _selectorVectorFindWhereNoteShouldBe(selector, note);
    int existIdx = _selectorVectorFindNoteLinearlyAfter(selector, note, idx);
    if (existIdx>=0) return;
    if (selector->capacity<=selector->notesNum && _selectorVectorDuplicateCapacity(selector)) return;
    (selector->notes)[selector->notesNum]=note;
    _selectorVectorUpdateValuesOnNoteAddition(selector, note);
    (selector->notesNum)++;
}

static void _selectorVectorAdd(struct notes_selection* selector, Note note) {
    if (_initSelectorVector(selector) || !note) return;
    //(selector->notes)[(selector->notesNum)++] = note;
    //_selectorVectorSort(selector);
    int idx = _selectorVectorFindWhereNoteShouldBe(selector, note);
    int existIdx = _selectorVectorFindNoteLinearlyAfter(selector, note, idx);

    if (existIdx>=0) return;
    if (selector->capacity<=selector->notesNum && _selectorVectorDuplicateCapacity(selector)) return;
    memmove(selector->notes+idx+1, selector->notes+idx, (selector->notesNum-idx)*sizeof(Note));
    (selector->notes)[idx]=note;
    _selectorVectorUpdateValuesOnNoteAddition(selector, note);
    (selector->notesNum)++;
}

static void _selectorVectorRemove(struct notes_selection* selector, Note note) {
    if (_initSelectorVector(selector) || !note) return;
    int idx=_selectorVectorFind(selector, note), num=selector->notesNum;
    if (idx<0) return;
    (selector->notes)[idx]=NULL;
    if (num-idx-1) memmove(selector->notes+idx, selector->notes+idx+1, (num-idx-1)*sizeof(Note));
    if (((selector->capacity)>>2)>(--(selector->notesNum))) _selectorVectorHalveCapacity(selector);
    _selectorVectorUpdateValuesOnNoteRemoval(selector, note);
}

static void _selectorVectorToggle(struct notes_selection* selector, Note note) {
    if (_initSelectorVector(selector) || !note) return;
    int idx = _selectorVectorFindWhereNoteShouldBe(selector, note), num=selector->notesNum;;
    int existIdx = _selectorVectorFindNoteLinearlyAfter(selector, note, idx);
    if (existIdx>=0) {
        (selector->notes)[existIdx]=NULL;
        if (num-existIdx-1) memmove(selector->notes+existIdx, selector->notes+existIdx+1, (num-existIdx-1)*sizeof(Note));
        if (((selector->capacity)>>2)>(--(selector->notesNum))) _selectorVectorHalveCapacity(selector);
        _selectorVectorUpdateValuesOnNoteRemoval(selector, note);
    } else {
        if (selector->capacity<=(uint32_t)num && _selectorVectorDuplicateCapacity(selector)) return;
        memmove(selector->notes+idx+1, selector->notes+idx, (num-idx)*sizeof(Note));
        (selector->notes)[idx]=note;
        _selectorVectorUpdateValuesOnNoteAddition(selector, note);
        (selector->notesNum)++;
    }
}

static struct roll_rect _selectorGetRollRect(struct notes_selection* selector) {
    if (!selector || !(selector->clickHold)) return (struct roll_rect){.topLeft=(struct roll_point){.fkey=-5, .timestamp=0}, .bottomRight=(struct roll_point){.fkey=-5, .timestamp=0}};
    struct roll_rect rr = selector->rollRect;
    struct roll_point topLeft = {.fkey=floatMax(rr.topLeft.fkey, rr.bottomRight.fkey), .timestamp=uint32Min(rr.topLeft.timestamp, rr.bottomRight.timestamp)};
    struct roll_point bottomRight = {.fkey=floatMin(rr.topLeft.fkey, rr.bottomRight.fkey), .timestamp=uint32Max(rr.topLeft.timestamp, rr.bottomRight.timestamp)};
    return (struct roll_rect){.topLeft=topLeft, .bottomRight=bottomRight};
}

static struct roll_rect _selectorGetValueRect(struct notes_selection* selector) {
    if (!selector || !(selector->notesNum)) return (struct roll_rect){.topLeft=(struct roll_point){.fkey=-5, .timestamp=0}, .bottomRight=(struct roll_point){.fkey=-5, .timestamp=0}};
    return (struct roll_rect){.topLeft=(struct roll_point){.fkey=selector->keyMax+1, .timestamp=selector->timestampStart}, .bottomRight=(struct roll_point){.fkey=selector->keyMin, .timestamp=selector->timestampEnd}};
}

static void _selectorVectorSelectRegion(struct notes_selection* selector, uint8_t minKey, uint8_t maxKey, uint32_t minTimestamp, uint32_t maxTimestamp) {   // not included in primary selector
    if (!selector || selector->notes || minKey>127 || minKey>maxKey) return;

    //printf("_selectorVectorSelectRegion(): selector=%p, minKey=%u, maxKey=%u, minTimestamp=%u, maxTimestamp=%u\n", (void*)selector, minKey, maxKey, minTimestamp, maxTimestamp);

    Track track = globalStateHandler->project->tracks+globalStateHandler->selectedTrack;
    Note* trNotes = track->notes;
    uint32_t trackLen=track->numElements;
    uint32_t trackIdx=0;//_tracksGetTimestampIdx(track, minTimestamp);

    while (trackIdx<trackLen && (trNotes[trackIdx]->timestamp)<maxTimestamp) {
        Note note = trNotes[trackIdx++];
        if (note->key>maxKey || note->key<minKey || note->timestamp+note->duration<minTimestamp) continue;
        int existIdx = _selectorVectorFind(&(globalStateHandler->selector.primary), note);
        if (existIdx<0) _selectorVectorAddLast(selector, note);
    }
}

static void _selectorVectorUpdateGroupSelected(struct notes_selection* selector) {
    //printf("selector=%p, topLeft.fkey=%.2f, bottomRight.fkey=%.2f\n", (void*)selector, selector->rollRect.topLeft.fkey, selector->rollRect.bottomRight.fkey);
    if (!selector) return;
    if (selector->rollRect.topLeft.fkey<0 || selector->rollRect.bottomRight.fkey<0) {
        _selectorVectorReset(selector);
        return;
    }

    if (selector->notes) free(selector->notes);
    selector->notes = NULL;
    selector->capacity = 0;
    selector->notesNum = 0;

    uint8_t rangeKeyMin, rangeKeyMax;
    struct roll_rect rr = _selectorGetRollRect(selector);
    rangeKeyMin = (uint8_t)floor(rr.bottomRight.fkey);
    rangeKeyMax = (uint8_t)floor(rr.topLeft.fkey);

    _selectorVectorSelectRegion(selector, rangeKeyMin, rangeKeyMax, rr.topLeft.timestamp, rr.bottomRight.timestamp);
}

void _mergeSortedVectors(Note* dest, const Note* a, uint32_t sa, const Note* b, uint32_t sb) {
    uint32_t id=0, sd=sa+sb, ia=0, ib=0;
    for (; id<sd; id++) {
        if (ia>=sa) dest[id]=b[ib++];
        else if (ib>=sb) dest[id]=a[ia++];
        else if (a[ia]->timestamp>b[ib]->timestamp) dest[id]=b[ib++];
        else dest[id]=a[ia++];
    }
}

static void _selectorsMergeAndClearNoChecks() {
    if (!globalStateHandler) return;
    struct notes_selection* primary = &(globalStateHandler->selector.primary);
    struct notes_selection* secondary = &(globalStateHandler->selector.secondary);

    if (!(secondary->notesNum)) {
        //printf("Secondary doesn't have any notes\n");
        _selectorVectorReset(secondary);
        return;
    }

    uint32_t num = primary->notesNum + secondary->notesNum;
    if (!num) return;

    if (!(primary->notes)) {

        primary->capacity = secondary->capacity;
        primary->notesNum = secondary->notesNum;
        primary->notes = secondary->notes;

        primary->keyMax = secondary->keyMax;
        primary->keyMin = secondary->keyMin;
        primary->timestampStart = secondary->timestampStart;
        primary->timestampEnd = secondary->timestampEnd;

        // No sorting needed, since it's supposed to be already sorted in the secondary selector
        secondary->notes = NULL;
        secondary->notesNum = (secondary->capacity=0);
        _selectorVectorReset(secondary);
        
    } else {
        Note* arr = malloc(num*sizeof(Note));
        if (!arr) return;   // Failed

        _mergeSortedVectors(arr, primary->notes, primary->notesNum, secondary->notes, secondary->notesNum);     // In O(n) instead of O(nlog(n)) using qsort
        
        if (primary->notesNum) {
            if (primary->keyMax<secondary->keyMax) primary->keyMax = secondary->keyMax;
            if (primary->keyMin>secondary->keyMin) primary->keyMin = secondary->keyMin;
            if (primary->timestampStart>secondary->timestampStart) primary->timestampStart = secondary->timestampStart;
            if (primary->timestampEnd<secondary->timestampEnd) primary->timestampEnd = secondary->timestampEnd;
        } else {
            primary->keyMax = secondary->keyMax;
            primary->keyMin = secondary->keyMin;
            primary->timestampStart = secondary->timestampStart;
            primary->timestampEnd = secondary->timestampEnd;
        }

        primary->capacity = num;
        primary->notesNum = num;
        free(primary->notes);
        primary->notes = arr;


        _selectorVectorReset(secondary);
    }
}

static void _selectorMoveClick(struct notes_selection* selector, Note note, float fkey, uint32_t timestamp) {
    if (!selector || !note || !(selector->notes) || fkey>=128 || fkey<0) return;
    selector->rollRect.topLeft = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    selector->rollRect.bottomRight = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    selector->clickHold = 1;
    selector->noteReference.note = note;
    selector->noteReference.originalTimestamp = note->timestamp;
    selector->noteReference.originalKey = note->key;
    selector->noteReference.minPossibleTimestamp = note->timestamp-selector->timestampStart;
    selector->noteReference.minPossibleKey = note->key-selector->keyMin;
    selector->noteReference.maxPossibleKey = note->key+(127-selector->keyMax);
}

static void _selectorMoveHold(struct notes_selection* selector, float fkey, uint32_t timestamp) {
    if (!selector || !(selector->notes) || selector->clickHold!=1 || !(selector->noteReference.note) || fkey>=128 || fkey<0) return;
    selector->rollRect.bottomRight = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    
    Note note = selector->noteReference.note;
    int tarKey = (int)(selector->noteReference.originalKey)+(int)roundf(fkey-selector->rollRect.topLeft.fkey);
    tarKey = intClip(tarKey, (int)(selector->noteReference.minPossibleKey), (int)(selector->noteReference.maxPossibleKey));
    uint8_t tkey = (uint8_t)tarKey;

    int64_t tmpTst = globalStateHandler->time.mouseJumps*(int64_t)round(((int)timestamp-(int)(selector->rollRect.topLeft.timestamp)+(int)(selector->noteReference.originalTimestamp))/(double)(globalStateHandler->time.mouseJumps));
    if (tmpTst<0) tmpTst=0;
    uint32_t tarTimestamp = (uint32_t)tmpTst;
    tarTimestamp = uint32Max(tarTimestamp, selector->noteReference.minPossibleTimestamp);

    if (note->key != tkey || note->timestamp!=tarTimestamp) {
        int dk = tarKey-note->key;
        int64_t dt = (int64_t)tarTimestamp-(int64_t)note->timestamp;

        uint32_t n=selector->notesNum;
        for (uint32_t i=0; i<n; i++) {
            Note nt = (selector->notes)[i];
            if (dk) nt->key = (uint8_t)(dk+nt->key);
            if (dt) nt->timestamp = (uint32_t)(dt+nt->timestamp);
        }

        selector->keyMin = (uint8_t)(tarKey-selector->noteReference.minPossibleKey);
        selector->keyMax = (uint8_t)(tarKey+127-selector->noteReference.maxPossibleKey);
        selector->timestampStart = (uint32_t)(dt+selector->timestampStart);
        selector->timestampEnd = (uint32_t)(dt+selector->timestampEnd);

        trackSortNotes(globalStateHandler->project->tracks+globalStateHandler->selectedTrack);

        projectUpdateStateSomethingChanged();
    }
}


static void _selectorResizeClick(struct notes_selection* selector, Note note, float fkey, uint32_t timestamp) {
    if (!selector || !note || !(selector->notes) || fkey>=128 || fkey<0) return;
    selector->rollRect.topLeft = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    selector->rollRect.bottomRight = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    selector->clickHold = 2;
    selector->noteReference.note = note;
    selector->noteReference.originalTimestamp = note->timestamp+note->duration;
    selector->noteReference.originalKey = note->key;
    selector->noteReference.minPossibleTimestamp = (1<<9);
    selector->noteReference.minPossibleKey = note->key-selector->keyMin;
    selector->noteReference.maxPossibleKey = note->key+(127-selector->keyMax);

    uint32_t n=selector->notesNum, minTst=-1;
    for (uint32_t i=0; i<n; i++) {
        Note nt = (selector->notes)[i];
        if (nt->duration<minTst) minTst=nt->duration;
    }
    selector->noteReference.minPossibleTimestamp += note->timestamp+note->duration-minTst;
}

static void _selectorResizeHold(struct notes_selection* selector, float fkey, uint32_t timestamp) {
    if (!selector || !(selector->notes) || selector->clickHold!=2 || !(selector->noteReference.note) || fkey>=128 || fkey<0) return;
    selector->rollRect.bottomRight = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    
    Note note = selector->noteReference.note;

    int64_t tmpTst = globalStateHandler->time.mouseJumps*(int64_t)round(((int)timestamp-(int)(selector->rollRect.topLeft.timestamp)+(int)(selector->noteReference.originalTimestamp))/(double)(globalStateHandler->time.mouseJumps));
    if (tmpTst<0) tmpTst=0;
    uint32_t tarTimestamp = (uint32_t)tmpTst;
    tarTimestamp = uint32Max(tarTimestamp, selector->noteReference.minPossibleTimestamp);

    if (note->timestamp+note->duration!=tarTimestamp) {
        int64_t dt = (int64_t)tarTimestamp-(int64_t)(note->timestamp+note->duration);

        uint32_t n=selector->notesNum;
        for (uint32_t i=0; i<n; i++) {
            Note nt = (selector->notes)[i];
            nt->duration = (uint32_t)(dt+nt->duration);
        }

        selector->timestampEnd = (uint32_t)(dt+selector->timestampEnd);
        projectUpdateStateSomethingChanged();
    }
}

static void _selectorVectorSelectAll(struct notes_selection* selector) {
    if (!selector) return;
    _selectorVectorReset(selector);
    Track track = globalStateHandler->project->tracks+globalStateHandler->selectedTrack;
    uint32_t n = track->numElements;
    if (!n) return;

    Note* notes = malloc(n*sizeof(Note));
    if (!notes) return;
    selector->notes = notes;
    selector->capacity = n;
    selector->notesNum = n;

    uint8_t minKey=128, maxKey=0;
    uint32_t startTimestamp=(track->notes)[0]->timestamp, endTimestamp=0;

    for (uint32_t i=0; i<n; i++) {
        Note nt = (track->notes)[i];
        (selector->notes)[i] = nt;
        if (minKey>nt->key) minKey=nt->key;
        if (maxKey<nt->key) maxKey=nt->key;
        if (endTimestamp<nt->timestamp+nt->duration) endTimestamp=nt->timestamp+nt->duration;
    }

    selector->keyMin = minKey;
    selector->keyMax = maxKey;
    selector->timestampStart = startTimestamp;
    selector->timestampEnd = endTimestamp;
}

static void _selectorDeleteSelected(struct notes_selection* selector) {
    if (!globalStateHandler || !selector || !(selector->notesNum) || globalStateHandler->selectedTrack<0 || !(globalStateHandler->project->tracks)) return;
    uint32_t num = selector->notesNum;
    int* indexes = malloc(num*sizeof(int));
    if (!indexes) return;

    Track track = globalStateHandler->project->tracks+globalStateHandler->selectedTrack;

    for (uint32_t i=0; i<num; i++) {
        indexes[i] = _tracksVectorFindWhereNoteShouldBe(track, (selector->notes)[i]);
    }
    qsort(indexes, num, sizeof(int), _intCompareFunc);
    uint32_t idx=0;
    while (idx<num && indexes[idx]<0) idx++;
    for (uint32_t i=idx; i<num; i++) free((track->notes)[indexes[i]]);

    uint32_t read=0, write=0, delete=idx;
    while (read<track->numElements) {
        if (delete<num && indexes[delete]==(int)read) {
            delete++;
            read++;
            continue;
        }
        (track->notes)[write++] = (track->notes)[read++];
    }
    track->numElements = write;

    free(indexes);
    trackVectorResizeToFitJustNotes(track);
    _selectorVectorReset(selector);
}

static void _copyNotes(Note* dest, const Note* source, uint32_t size) {
    if (!dest || !source || !size) return;

    for (uint32_t i=0; i<size; i++) {
        Note note = malloc(sizeof(struct note_data));
        if (!note) return;
        *note = *(source[i]);
        dest[i] = note;
    }
}

static void _selectorVectorCopy(struct notes_selection* dest, struct notes_selection* source) {
    if (!dest || !source) return;
    if (&(globalStateHandler->selector.clipboard)==dest) _selectorVectorDeepFree(dest);
    else _selectorVectorReset(dest);
    
    uint32_t n=source->notesNum;
    if (!n) return;

    Note* arr = malloc(sizeof(Note)*n);
    if (!arr) return;

    _copyNotes(arr, source->notes, n);
    *dest = *source;
    dest->notes = arr;
}

static void _selectorVectorCut(struct notes_selection* dest, struct notes_selection* source) {
    if (!dest || !source) return;
    _selectorVectorDeepFree(dest);
    uint32_t n=source->notesNum;
    if (!n) return;

    _selectorVectorCopy(dest, source);      // Must be a better way of doing that
    _selectorDeleteSelected(source);
}

static void _selectorVectorPaste(struct notes_selection* dest, struct notes_selection* source) {
    if (!dest || !source) return;
    _selectorVectorReset(dest);

    uint32_t n=source->notesNum;
    if (!n) return;
    _selectorVectorCopy(dest, source);

    Track track = globalStateHandler->project->tracks+globalStateHandler->selectedTrack;
    trackVectorAddNewNotes(track, dest->notes, dest->notesNum);
}

void globalHandlerAddNoteToSelected(Note note) {
    if (!globalStateHandler || !note) return;
    _selectorVectorAdd(&(globalStateHandler->selector.primary), note);
}

void globalHandlerRemoveSelectedNote(Note note) {
    if (!globalStateHandler || !note) return;
    _selectorVectorRemove(&(globalStateHandler->selector.primary), note);
}

void globalHandlerToggleSelectedNote(Note note) {
    if (!globalStateHandler || !note) return;
    _selectorVectorToggle(&(globalStateHandler->selector.primary), note);
}

int globalHandlerIsNoteSelected(Note note) {
    if (!globalStateHandler || !note) return 0;
    if (_selectorVectorFind(&(globalStateHandler->selector.primary), note)>=0) return 1;
    if (_selectorVectorFind(&(globalStateHandler->selector.secondary), note)>=0) return 1;
    return 0;
}

int globalHandlerGetNumberOfActuallySelectedNotes() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->selector.primary.notesNum;
}

int globalHandlerGetNumberOfVisuallySelectedNotes() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->selector.primary.notesNum+globalStateHandler->selector.secondary.notesNum;
}

void globalHandlerDeleteSelectedNotes() {
    if (!globalStateHandler || !(globalStateHandler->selector.primary.notesNum) || globalStateHandler->selectedTrack<0 || !(globalStateHandler->project->tracks)) return;
    _selectorDeleteSelected(&(globalStateHandler->selector.primary));

    projectUpdateStateSomethingChanged();
}


void globalHandlerSelectGroupPress(float fkey, uint32_t timestamp) {
    if (!globalStateHandler || fkey>128 || fkey<0) return;
    globalStateHandler->selector.secondary.rollRect.topLeft = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    globalStateHandler->selector.secondary.rollRect.bottomRight = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    globalStateHandler->selector.secondary.clickHold = 1;
    _selectorVectorUpdateGroupSelected(&(globalStateHandler->selector.secondary));
}

void globalHandlerSelectGroupHold(float fkey, uint32_t timestamp) {
    if (!globalStateHandler || !(globalStateHandler->selector.secondary.clickHold)) return;
    if (fkey>=0 && fkey<128) globalStateHandler->selector.secondary.rollRect.bottomRight = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    globalStateHandler->selector.secondary.clickHold = 1;
    _selectorVectorUpdateGroupSelected(&(globalStateHandler->selector.secondary));
}

int globalHandlerIsSelectGroupActive() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->selector.secondary.clickHold;
}

struct roll_rect globalHandlerGetSelectGroupRect() {
    if (!globalStateHandler) return _selectorGetRollRect(NULL);
    return _selectorGetRollRect(&(globalStateHandler->selector.secondary));
}

void globalHandlerSelectGroupRelease(float fkey, uint32_t timestamp) {
    if (!globalStateHandler || !(globalStateHandler->selector.secondary.clickHold)) return;
    if (fkey>=0 && fkey<128) globalStateHandler->selector.secondary.rollRect.bottomRight = (struct roll_point){.fkey=fkey, .timestamp=timestamp};
    _selectorVectorUpdateGroupSelected(&(globalStateHandler->selector.secondary));
    globalStateHandler->selector.secondary.clickHold = 0;
    _selectorsMergeAndClearNoChecks();
}

void globalHandlerSelectGroupClear() {
    if (!globalStateHandler || !(globalStateHandler->selector.secondary.clickHold)) return;
    _selectorVectorReset(&(globalStateHandler->selector.secondary));
}


struct roll_rect globalHandlerGetCroppedRectangleForSelectedNotes() {
    if (!globalStateHandler) return _selectorGetValueRect(NULL);
    return _selectorGetValueRect(&(globalStateHandler->selector.primary));
}

uint32_t globalHandlerGetNumberOfSelectedNotes() {
    if (!globalStateHandler) return 0;
    return globalStateHandler->selector.primary.notesNum;
}


void globalHandlerSelectSingleNote(Note note) {
    if (!globalStateHandler || !note) return;
    _selectorVectorReset(&(globalStateHandler->selector.primary));
    _selectorVectorAdd(&(globalStateHandler->selector.primary), note);
}

void globalHandlerChangeVelocityOfSelectedNotes(uint8_t velocity) {
    if (!globalStateHandler || velocity>127) return;
    if (globalStateHandler->selector.primary.notesNum==1 && globalStateHandler->selector.primary.notes) (globalStateHandler->selector.primary.notes)[0]->velocity = velocity;
    // else run a more general function that applies it to all
}


void globalHandlerMoveSelectedPress(Note note, float fkey, uint32_t timestamp) {
    if (!globalStateHandler || fkey>128 || fkey<0 || !note) return;
    _selectorMoveClick(&(globalStateHandler->selector.primary), note, fkey, timestamp);
}

void globalHandlerMoveSelectedHold(float fkey, uint32_t timestamp) {
    if (!globalStateHandler || fkey>128 || fkey<0) return;
    _selectorMoveHold(&(globalStateHandler->selector.primary), fkey, timestamp);
}

void globalHandlerMoveSelectedRelease(float fkey, uint32_t timestamp) {
    if (!globalStateHandler || fkey>128 || fkey<0) return;
    _selectorMoveHold(&(globalStateHandler->selector.primary), fkey, timestamp);
    globalStateHandler->selector.primary.clickHold = 0;
    globalStateHandler->selector.primary.noteReference = (struct reference_note){NULL,};
}

int globalHandlerMoveSelectedIsActive() {
    if (!globalStateHandler || !(globalStateHandler->selector.primary.notesNum)) return 0;
    return (globalStateHandler->selector.primary.clickHold==1);
}

void globalHandlerResizeSelectedPress(Note note, float fkey, uint32_t timestamp) {
    if (!globalStateHandler || fkey>128 || fkey<0 || !note) return;
    _selectorResizeClick(&(globalStateHandler->selector.primary), note, fkey, timestamp);
}

void globalHandlerResizeSelectedHold(float fkey, uint32_t timestamp) {
    if (!globalStateHandler || fkey>128 || fkey<0) return;
    _selectorResizeHold(&(globalStateHandler->selector.primary), fkey, timestamp);
}

uint32_t globalHandlerResizeSelectedRelease(float fkey, uint32_t timestamp) {
    if (!globalStateHandler || fkey>128 || fkey<0) return (1<<10);
    _selectorResizeHold(&(globalStateHandler->selector.primary), fkey, timestamp);
    globalStateHandler->selector.primary.clickHold = 0;
    uint32_t ret=(1<<10);
    if (globalStateHandler->selector.primary.noteReference.note) ret=globalStateHandler->selector.primary.noteReference.note->duration;
    globalStateHandler->selector.primary.noteReference = (struct reference_note){NULL,};
    return ret;
}

int globalHandlerResizeSelectedIsActive() {
    if (!globalStateHandler || !(globalStateHandler->selector.primary.notesNum)) return 0;
    return (globalStateHandler->selector.primary.clickHold==2);
}

Note globalHandlerResizeSelectedGetReferenceNote() {
    if (!globalStateHandler || !(globalStateHandler->selector.primary.notesNum) || globalStateHandler->selector.primary.clickHold!=2) return NULL;
    return globalStateHandler->selector.primary.noteReference.note;
}

void globalHandlerSelectAllNotes() {
    if (!globalStateHandler) return;
    _selectorVectorSelectAll(&(globalStateHandler->selector.primary));
}

void globalHandlerClearClipboard() {
    if (!globalStateHandler) return;
    _selectorVectorDeepFree(&(globalStateHandler->selector.clipboard));
}

void globalHandlerCopySelected() {
    if (!globalStateHandler || !(globalStateHandler->selector.primary.notesNum)) return;
    _selectorVectorCopy(&(globalStateHandler->selector.clipboard), &(globalStateHandler->selector.primary));
}

void globalHandlerCutSelected() {
    if (!globalStateHandler || !(globalStateHandler->selector.primary.notesNum)) return;
    _selectorVectorCut(&(globalStateHandler->selector.clipboard), &(globalStateHandler->selector.primary));
    projectUpdateStateSomethingChanged();
}

void globalHandlerPasteSelected() {
    if (!globalStateHandler || !(globalStateHandler->selector.clipboard.notesNum)) return;
    _selectorVectorPaste(&(globalStateHandler->selector.primary), &(globalStateHandler->selector.clipboard));
    projectUpdateStateSomethingChanged();
}