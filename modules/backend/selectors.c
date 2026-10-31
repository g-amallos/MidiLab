#include "backend_internal.h"
#include <utils.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sortedVector.h>



static void updateSelectedTracksValues() {
    if (!globalStateHandler || !globalProject || !(globalProject->tracks) || globalProject->tracksNum<=0) return;
    int idx = globalStateHandler->selectedTrack;
    if (idx<0 || idx>=globalProject->tracksNum) return;
    trackRecalculateValues(globalProject->tracks+idx);
}


static void _freeNoteSelector() {
    if (globalStateHandler->selector.primary.notes) {
        sortedVectorDeleteAllElements(globalStateHandler->selector.primary.notes);
    }
    if (globalStateHandler->selector.secondary.notes) {
        sortedVectorDeleteAllElements(globalStateHandler->selector.secondary.notes);
    }

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
    };

    struct notes_selection t1=tmpSel, t2=tmpSel;
    t1.notes = globalStateHandler->selector.primary.notes;
    t2.notes = globalStateHandler->selector.secondary.notes;

    globalStateHandler->selector.primary = t1;
    globalStateHandler->selector.secondary = t2;
}

void globalHandlerClearNotesSelected() {
    if (!globalStateHandler) return;
    _freeNoteSelector();
}

static void _selectorVectorReset(struct notes_selection* selector) {
    if (!selector) return;
    sortedVectorDeleteAllElements(selector->notes);
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
        .notes = selector->notes
    };
}


static void _selectorVectorDeepFree(struct notes_selection* selector) {
    if (!selector) return;
    sortedVectorSetDestroyFunc(selector->notes, _destroyNote);
    _selectorVectorReset(selector);
    sortedVectorSetDestroyFunc(selector->notes, NULL);
}

static int _initSelectorVector(struct notes_selection* selector) {
    if (!selector) return 1;
    if (!(selector->notes)) {
        selector->notes = sortedVectorCreate(_compareNotes, NULL);
    }
    return !(selector->notes);
}

void selectorVectorsClose() {
    if (!globalStateHandler) return;
    sortedVectorDestroy(globalStateHandler->selector.primary.notes);
    sortedVectorDestroy(globalStateHandler->selector.secondary.notes);
    sortedVectorSetDestroyFunc(globalStateHandler->selector.clipboard.notes, _destroyNote);
    sortedVectorDestroy(globalStateHandler->selector.clipboard.notes);
}


static void _selectorVectorUpdateValuesOnNoteAddition(struct notes_selection* selector, Note note) {
    if (!selector || !(selector->notes)) return;
    if ((sortedVectorGetSize(selector->notes))<=1) {
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
    if (!selector || !(selector->notes)) return;

    uint32_t num = sortedVectorGetSize(selector->notes);
    if (!num) {
        selector->keyMin=0;
        selector->keyMax=0;
        selector->timestampStart=0;
        selector->timestampEnd=0;
        return;
    }

    const Note* arr = (const Note*)sortedVectorGetElements(selector->notes);
    selector->timestampStart = arr[0]->timestamp;
    uint8_t keyMin=128, keyMax=0;
    uint32_t timestampEnd=0, idx=0;
    for (; idx<num; idx++) {
        Note nt = arr[idx];
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
    if (!(sortedVectorGetSize(selector->notes)) || note->key<=selector->keyMin || note->key>=selector->keyMax || note->timestamp<=selector->timestampStart || note->timestamp+note->duration>=selector->timestampEnd) _selectorVectorUpdateValues(selector);
}


static void _selectorVectorToggle(struct notes_selection* selector, Note note) {
    if (_initSelectorVector(selector) || !note) return;
    int existIdx = sortedVectorFindElement(selector->notes, (const void*)note);
    if (existIdx>=0) {
        sortedVectorDeleteElementAt(selector->notes, existIdx);
        _selectorVectorUpdateValuesOnNoteRemoval(selector, note);
    } else {
        sortedVectorAdd(selector->notes, note);
        _selectorVectorUpdateValuesOnNoteAddition(selector, note);
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
    if (!selector || !(sortedVectorGetSize(selector->notes))) return (struct roll_rect){.topLeft=(struct roll_point){.fkey=-5, .timestamp=0}, .bottomRight=(struct roll_point){.fkey=-5, .timestamp=0}};
    //printf("Selector: keyMin: %d, keyMax: %d, tmstMin: %u, tmstMax: %u\n", selector->keyMin, selector->keyMax+1, selector->timestampStart, selector->timestampEnd);
    return (struct roll_rect){.topLeft=(struct roll_point){.fkey=selector->keyMax+1, .timestamp=selector->timestampStart}, .bottomRight=(struct roll_point){.fkey=selector->keyMin, .timestamp=selector->timestampEnd}};
}

static void _selectorVectorSelectRegion(struct notes_selection* selector, uint8_t minKey, uint8_t maxKey, uint32_t minTimestamp, uint32_t maxTimestamp) {   // not included in primary selector
    if (!selector || _initSelectorVector(selector) || !(selector->notes) || minKey>127 || minKey>maxKey) return;

    //printf("_selectorVectorSelectRegion(): selector=%p, minKey=%u, maxKey=%u, minTimestamp=%u, maxTimestamp=%u\n", (void*)selector, minKey, maxKey, minTimestamp, maxTimestamp);

    Track track = globalStateHandler->project->tracks+globalStateHandler->selectedTrack;
    const Note* trNotes = (const Note*)sortedVectorGetElements(track->notes);
    uint32_t trackLen=sortedVectorGetSize(track->notes);
    uint32_t trackIdx=0;//_tracksGetTimestampIdx(track, minTimestamp);

    while (trackIdx<trackLen && (trNotes[trackIdx]->timestamp)<maxTimestamp) {
        Note note = trNotes[trackIdx++];
        if (note->key>maxKey || note->key<minKey || note->timestamp+note->duration<minTimestamp) continue;
        int existIdx = sortedVectorFindElement(globalStateHandler->selector.primary.notes, note);
        if (existIdx<0) sortedVectorAdd(selector->notes, note);
    }
}

static void _selectorVectorUpdateGroupSelected(struct notes_selection* selector) {
    //printf("selector=%p, topLeft.fkey=%.2f, bottomRight.fkey=%.2f\n", (void*)selector, selector->rollRect.topLeft.fkey, selector->rollRect.bottomRight.fkey);
    if (!selector) return;
    if (selector->rollRect.topLeft.fkey<0 || selector->rollRect.bottomRight.fkey<0) {
        _selectorVectorReset(selector);
        return;
    }

    sortedVectorDeleteAllElements(selector->notes);

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

    if (!(sortedVectorGetSize(secondary->notes))) {
        //printf("Secondary doesn't have any notes\n");
        _selectorVectorReset(secondary);
        return;
    }

    uint32_t num = sortedVectorGetSize(primary->notes)+sortedVectorGetSize(secondary->notes);
    if (!num) return;

    if (!(primary->notes)) {
        primary->notes = secondary->notes;

        primary->keyMax = secondary->keyMax;
        primary->keyMin = secondary->keyMin;
        primary->timestampStart = secondary->timestampStart;
        primary->timestampEnd = secondary->timestampEnd;

        // No sorting needed, since it's supposed to be already sorted in the secondary selector
        secondary->notes = NULL;
        _selectorVectorReset(secondary);
        
    } else {
        uint32_t old = sortedVectorGetSize(primary->notes);
        sortedVectorMerge(primary->notes, secondary->notes);
      
        if (old) {
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

        _selectorVectorReset(secondary);
    }
    _selectorVectorUpdateValues(primary);
}

static void _selectorVectorMoveByKey(struct notes_selection* selector, int keyOffSet) {
    if (!selector || !(selector->notes) || !(sortedVectorGetSize(selector->notes)) || (keyOffSet+selector->keyMax)>127 || (keyOffSet+selector->keyMin)<0) return;

    selector->keyMax = (uint8_t)(keyOffSet+selector->keyMax);
    selector->keyMin = (uint8_t)(keyOffSet+selector->keyMin);

    uint32_t n=sortedVectorGetSize(selector->notes);
    Note* arr = (Note*)sortedVectorGetElements(selector->notes);    // Allowed key edit
    for (uint32_t i=0; i<n; i++) {
        arr[i]->key = (uint8_t)(keyOffSet+arr[i]->key);
    }
}

static void _selectorVectorMoveByTimestamp(struct notes_selection* selector, int timestampOffSet) {
    if (!selector || !(selector->notes) || !(sortedVectorGetSize(selector->notes)) || (timestampOffSet+(int)(selector->timestampStart))<0) return;

    selector->timestampStart = (uint32_t)(timestampOffSet+selector->timestampStart);
    selector->timestampEnd = (uint32_t)(timestampOffSet+selector->timestampEnd);

    uint32_t n=sortedVectorGetSize(selector->notes);
    Note* arr = (Note*)sortedVectorGetElements(selector->notes);    // Allowed identical edit
    for (uint32_t i=0; i<n; i++) {
        arr[i]->timestamp = (uint32_t)(timestampOffSet+(int64_t)(arr[i]->timestamp));
    }
    trackSortNotes(globalStateHandler->project->tracks+globalStateHandler->selectedTrack);
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

        uint32_t n=sortedVectorGetSize(selector->notes);
        Note* arr=(Note*)sortedVectorGetElements(selector->notes);      // Allowed identical edits

        for (uint32_t i=0; i<n; i++) {
            Note nt = arr[i];
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

    uint32_t n=sortedVectorGetSize(selector->notes), minTst=-1;
    Note* arr = (Note*)sortedVectorGetElements(selector->notes);    // Allowed identical edits??
    for (uint32_t i=0; i<n; i++) {
        Note nt = arr[i];
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

        uint32_t n=sortedVectorGetSize(selector->notes);
        Note* arr = (Note*)sortedVectorGetElements(selector->notes);    // Allowed identical edits??
        for (uint32_t i=0; i<n; i++) {
            Note nt = arr[i];
            nt->duration = (uint32_t)(dt+nt->duration);
        }

        selector->timestampEnd = (uint32_t)(dt+selector->timestampEnd);
        projectUpdateStateSomethingChanged();
    }
}

static void _selectorVectorSelectAll(struct notes_selection* selector) {
    if (!selector || _initSelectorVector(selector)) return;
    
    Track track = globalStateHandler->project->tracks+globalStateHandler->selectedTrack;
    _selectorVectorReset(selector);
    sortedVectorCopyElementsFromVector(selector->notes, track->notes);

    uint32_t n = sortedVectorGetSize(selector->notes);
    if (!n) return;

    const Note* notes = (const Note*)sortedVectorGetElements(selector->notes);
    if (!notes) return;

    uint8_t minKey=128, maxKey=0;
    uint32_t startTimestamp=notes[0]->timestamp, endTimestamp=0;

    for (uint32_t i=0; i<n; i++) {
        Note nt = notes[i];
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
    if (!globalStateHandler || !selector || !(sortedVectorGetSize(selector->notes)) || globalStateHandler->selectedTrack<0 || !(globalStateHandler->project->tracks)) return;

    Track track = globalStateHandler->project->tracks+globalStateHandler->selectedTrack;
    sortedVectorDeleteFromVector(track->notes, selector->notes);
    _selectorVectorReset(selector);
}

static void _copyNotes(Note* dest, const Note* source, uint32_t size, int64_t timestampOffset) {
    if (!dest || !source || !size) return;

    for (uint32_t i=0; i<size; i++) {
        Note note = malloc(sizeof(struct note_data));
        if (!note) return;
        *note = *(source[i]);
        note->timestamp = (uint32_t)(timestampOffset+(int64_t)(note->timestamp));
        dest[i] = note;
    }
}

static void _selectorVectorCopy(struct notes_selection* dest, struct notes_selection* source, int64_t timestampOffset) {    // Continue from here!!!
    if (!dest || !source || source->timestampStart+timestampOffset<0) return;
    if (&(globalStateHandler->selector.clipboard)==dest) _selectorVectorDeepFree(dest);
    else _selectorVectorReset(dest);
    
    uint32_t n=sortedVectorGetSize(source->notes);
    if (!n) return;

    Note* arr = malloc(sizeof(Note)*n);
    if (!arr) return;

    _copyNotes(arr, (const Note*)sortedVectorGetElements(source->notes), n, timestampOffset);
    sortedVectorDestroy(dest->notes);
    dest->notes = sortedVectorCreateFromData(_compareNotes, NULL, n, (void**)arr);
    free(arr);

    printf("_selectorVectorCopy:  dest.keyMin=%d, dest.keyMax=%d, dest.timestampStart=%u, dest.timestampEnd=%u | source.keyMin=%d, source.keyMax=%d, source.timestampStart=%u, source.timestampEnd=%u  -->  \n", dest->keyMin, dest->keyMax, dest->timestampStart, dest->timestampEnd, source->keyMin, source->keyMax, source->timestampStart, source->timestampEnd);

    dest->keyMin = source->keyMin;
    dest->keyMax = source->keyMax;

    dest->timestampStart = (uint32_t)(timestampOffset+dest->timestampStart);
    dest->timestampEnd = (uint32_t)(timestampOffset+dest->timestampEnd);
    
    _selectorVectorUpdateValues(dest);
}

static void _selectorVectorCut(struct notes_selection* dest, struct notes_selection* source) {
    if (!dest || !source) return;
    _selectorVectorDeepFree(dest);
    uint32_t n=sortedVectorGetSize(source->notes);
    if (!n) return;

    _selectorVectorCopy(dest, source, 0);      // Must be a better way of doing that
    _selectorDeleteSelected(source);
}

static void _selectorVectorPaste(struct notes_selection* dest, struct notes_selection* source, uint32_t firstTimestamp) {
    if (!dest || !source) return;
    _selectorVectorReset(dest);

    uint32_t n=sortedVectorGetSize(source->notes);
    if (!n) return;
    _selectorVectorCopy(dest, source, ((int64_t)firstTimestamp)-source->timestampStart);

    Track track = globalStateHandler->project->tracks+globalStateHandler->selectedTrack;
    trackVectorAddNewNotes(track, dest->notes);
}

void globalHandlerAddNoteToSelected(Note note) {
    if (!globalStateHandler || !note) return;
    sortedVectorAdd(globalStateHandler->selector.primary.notes, note);
}

void globalHandlerRemoveSelectedNote(Note note) {
    if (!globalStateHandler || !note) return;
    sortedVectorDeleteElement(globalStateHandler->selector.primary.notes, note);
}

void globalHandlerToggleSelectedNote(Note note) {
    if (!globalStateHandler || !note) return;
    _selectorVectorToggle(&(globalStateHandler->selector.primary), note);
}

int globalHandlerIsNoteSelected(Note note) {
    if (!globalStateHandler || !note) return 0;
    if (sortedVectorFindElement(globalStateHandler->selector.primary.notes, note)>=0) return 1;
    if (sortedVectorFindElement(globalStateHandler->selector.secondary.notes, note)>=0) return 1;
    return 0;
}

int globalHandlerIsNoteActuallySelected(Note note) {
    if (!globalStateHandler || !note) return 0;
    if (sortedVectorFindElement(globalStateHandler->selector.primary.notes, note)>=0) return 1;
    return 0;
}

int globalHandlerGetNumberOfActuallySelectedNotes() {
    if (!globalStateHandler) return 0;
    return sortedVectorGetSize(globalStateHandler->selector.primary.notes);
}

int globalHandlerGetNumberOfVisuallySelectedNotes() {
    if (!globalStateHandler) return 0;
    return sortedVectorGetSize(globalStateHandler->selector.primary.notes)+sortedVectorGetSize(globalStateHandler->selector.secondary.notes);
}

void globalHandlerDeleteSelectedNotes() {
    if (!globalStateHandler || !(sortedVectorGetSize(globalStateHandler->selector.primary.notes)) || globalStateHandler->selectedTrack<0 || !(globalStateHandler->project->tracks)) return;
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
    return sortedVectorGetSize(globalStateHandler->selector.primary.notes);
}


void globalHandlerSelectSingleNote(Note note) {
    if (!globalStateHandler || !note) return;
    _selectorVectorReset(&(globalStateHandler->selector.primary));
    sortedVectorAdd(globalStateHandler->selector.primary.notes, note);
}

void globalHandlerChangeVelocityOfSelectedNotes(uint8_t velocity) {
    if (!globalStateHandler || velocity>127) return;
    if (sortedVectorGetSize(globalStateHandler->selector.primary.notes)==1 && globalStateHandler->selector.primary.notes) {
        Note* nt = (Note*)sortedVectorGetElements(globalStateHandler->selector.primary.notes);
        nt[0]->velocity = velocity;
    }
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
    updateSelectedTracksValues();
}

int globalHandlerMoveSelectedIsActive() {
    if (!globalStateHandler || !(sortedVectorGetSize(globalStateHandler->selector.primary.notes))) return 0;
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
    updateSelectedTracksValues();
    return ret;
}

int globalHandlerResizeSelectedIsActive() {
    if (!globalStateHandler || !(sortedVectorGetSize(globalStateHandler->selector.primary.notes))) return 0;
    return (globalStateHandler->selector.primary.clickHold==2);
}

Note globalHandlerResizeSelectedGetReferenceNote() {
    if (!globalStateHandler || !(sortedVectorGetSize(globalStateHandler->selector.primary.notes)) || globalStateHandler->selector.primary.clickHold!=2) return NULL;
    return globalStateHandler->selector.primary.noteReference.note;
}

const Note* globalHandlerGetSelectedNotes() {
    if (!globalStateHandler || !(sortedVectorGetSize(globalStateHandler->selector.primary.notes)) || !(globalStateHandler->selector.primary.notes)) return NULL;
    return (const Note*)sortedVectorGetElements(globalStateHandler->selector.primary.notes);
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
    if (!globalStateHandler || !(sortedVectorGetSize(globalStateHandler->selector.primary.notes))) return;
    _selectorVectorCopy(&(globalStateHandler->selector.clipboard), &(globalStateHandler->selector.primary), 0);
}

void globalHandlerCutSelected() {
    if (!globalStateHandler || !(sortedVectorGetSize(globalStateHandler->selector.primary.notes))) return;
    _selectorVectorCut(&(globalStateHandler->selector.clipboard), &(globalStateHandler->selector.primary));
    projectUpdateStateSomethingChanged();
    updateSelectedTracksValues();
}

void globalHandlerPasteSelected() {
    if (!globalStateHandler || !(sortedVectorGetSize(globalStateHandler->selector.clipboard.notes))) return;
    _selectorVectorPaste(&(globalStateHandler->selector.primary), &(globalStateHandler->selector.clipboard), globalHandlerGetLineTimestamp());
    globalHandlerSetLineTimestamp(globalStateHandler->selector.primary.timestampEnd);
    globalHandlerMoveTimeDivAccordingToTimeLine(0.2, 0.2);
    projectUpdateStateSomethingChanged();
    updateSelectedTracksValues();
}

void globalHandlerMoveSelectedPitch(int keyOffset) {
    if (!globalStateHandler || (globalStateHandler->selector.primary.clickHold)) return;
    _selectorVectorMoveByKey(&(globalStateHandler->selector.primary), keyOffset);
    projectUpdateStateSomethingChanged();
    updateSelectedTracksValues();
}

void globalHandlerMoveSelectedTimestamp(int timestampOffset) {
    if (!globalStateHandler || (globalStateHandler->selector.primary.clickHold)) return;
    _selectorVectorMoveByTimestamp(&(globalStateHandler->selector.primary), timestampOffset);
    projectUpdateStateSomethingChanged();
    updateSelectedTracksValues();
}