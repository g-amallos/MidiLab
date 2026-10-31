#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sortedVector.h>
#include <stdio.h>


#define DEFAULT_SIZE 8


struct sorted_vector {
    uint32_t size;
    uint32_t capacity;

    void** vector;
    CompareFunc compare;
    DestroyFunc destroy;
};


static int creations=0, deletions=0;

SortedVector sortedVectorCreate(CompareFunc compare, DestroyFunc destroy) {
    if (!compare) return NULL;
    SortedVector ret = malloc(sizeof(struct sorted_vector));
    if (!ret) return NULL;

    ret->size = 0;
    ret->capacity = 0;
    ret->vector = NULL;
    ret->compare = compare;
    ret->destroy = destroy;

    creations++;

    return ret;
}

SortedVector sortedVectorCreateFromData(CompareFunc compare, DestroyFunc destroy, uint32_t num, void** data) {
    fprintf(stderr, "`sortedVectorCreateFromData`: *may* contain NULL bytes\n");
    if (!compare) return NULL;
    SortedVector ret = malloc(sizeof(struct sorted_vector));
    if (!ret) return NULL;

    ret->compare = compare;
    ret->destroy = destroy;

    if (!num || !data) {
        ret->size = 0;
        ret->capacity = 0;
        ret->vector = NULL;
        
    } else {
        ret->vector = malloc(sizeof(void*)*num);
        if (!(ret->vector)) {
            free(ret);
            return NULL;
        }
        memcpy(ret->vector, data, sizeof(void*)*num);
        ret->size=num;
        ret->capacity=num;

        sortedVectorSort(ret);
    }

    creations++;

    return ret;
}

uint32_t sortedVectorGetSize(SortedVector svec) {
    if (!svec) return 0;
    return svec->size;
}

uint32_t sortedVectorGetCapacity(SortedVector svec) {
    if (!svec) return 0;
    return svec->capacity;
}

uint32_t sortedVectorGetFreeSpace(SortedVector svec) {
    if (!svec) return 0;
    return svec->capacity-svec->size;
}

const void** sortedVectorGetElements(SortedVector svec) {
    if (!svec) return NULL;
    return (const void**)(svec->vector);
}


static int _sortedVectorInit(SortedVector svec) {
    if (!svec) return 1;
    if (!(svec->vector)) {
        uint32_t cap = DEFAULT_SIZE;
        svec->vector = calloc(cap, sizeof(void*));
        svec->capacity = (svec->vector)?cap:0;
        svec->size = 0;
    }
    return !(svec->vector);
}

static int _sortedVectorDuplicateCapacity(SortedVector svec) {
    uint32_t oldc = svec->capacity;
    uint32_t newc = (oldc<<1);

    void** arr = realloc(svec->vector, newc*sizeof(void*));
    if (!arr) return 1;

    svec->capacity = newc;
    svec->vector = arr;
    return 0;
}

static int _sortedVectorDuplicateCapacityIfNeeded(SortedVector svec) {
    if (_sortedVectorInit(svec)) return 1;
    if (svec->size>=svec->capacity && _sortedVectorDuplicateCapacity(svec)) return 1;
    return 0;
}

static int _sortedVectorHalveCapacity(SortedVector svec) {
    uint32_t oldc = svec->capacity;
    uint32_t newc = (oldc>>1);
    if (newc<DEFAULT_SIZE) newc=DEFAULT_SIZE;

    void** arr = realloc(svec->vector, newc*sizeof(void*));
    if (!arr) return 1;

    svec->capacity = newc;
    svec->vector = arr;
    return 0;
}

static int _sortedVectorHalveCapacityIfNeeded(SortedVector svec) {
    if (_sortedVectorInit(svec)) return 1;
    if ((svec->size<<2)<svec->capacity && svec->capacity>DEFAULT_SIZE && _sortedVectorHalveCapacity(svec)) return 1;
    return 0;
}

static const void* _sortedVectorGetElementAt(SortedVector svec, uint32_t idx) {
    if (!svec || svec->size<=idx || !(svec->vector)) return NULL;
    return (svec->vector)[idx];
}

static uint32_t _sortedVectorGetElementIdxLogLeft(SortedVector svec, const void* elm) {    // Returns the idx of the starting index of where the element might be
    if (!svec || !elm) return -1;   // Idk what else lol
    uint32_t left=0, right=svec->size, middle;

    while (left<right) {       // Find the first index of the array which gives compare==0 (might have multiple such elements) 
        middle=left+(right-left)/2;
        void* telm = (svec->vector)[middle];
        int res = (svec->compare)(&telm, &elm);
        if (res<0) left=middle+1;
        else right=middle;
    }

    if (left<svec->size && !(svec->compare)(svec->vector+left, &elm)) return left;
    return -1;
}

static uint32_t _sortedVectorGetElementIdxLogRight(SortedVector svec, const void* elm) {    // Returns the idx of the first element that compares >0
    if (!svec || !elm) return -1;
    uint32_t left=0, right=svec->size, middle;

    while (left<right) {
        middle=left+(right-left)/2;
        void* telm = (svec->vector)[middle];
        int res = (svec->compare)(&telm, &elm);
        if (res<=0) left=middle+1;
        else right=middle;
    }

    if (left<svec->size && (svec->compare)(svec->vector+left, &elm)>0) return left;
    else if (left==svec->size) return svec->size;
    return -1;
}

static uint32_t _sortedVectorGetElementIdxLin(SortedVector svec, const void* elm, uint32_t stidx) {
    if (!svec || !elm || stidx==(uint32_t)(-1)) return -1;
    uint32_t size = svec->size;

    while (1) {
        if (stidx>=size) return -1;             // Again, not found
        void* telm = (svec->vector)[stidx];
        if (telm==elm) return stidx;

        int res = (svec->compare)(&telm, &elm);
        if (res!=0) return -1;
        stidx++;
    }
    return -1;
}

static uint32_t _sortedVectorGetElementIdx(SortedVector svec, const void* elm) {
    uint32_t idx = _sortedVectorGetElementIdxLogLeft(svec, elm);
    return _sortedVectorGetElementIdxLin(svec, elm, idx);
}

static int _sortedVectorFindElement(SortedVector svec, const void* elm) {
    if (_sortedVectorInit(svec)) return -1;
    uint32_t idx = _sortedVectorGetElementIdx(svec, elm);
    if (idx==(uint32_t)(-1)) return -1;
    return (int)idx;
}

void _debug(SortedVector svec) {
    if (!svec) return;
    printf("SortedVector (add): %p, %3u\n", (void*)svec, svec->size);
    for (uint32_t i=0; i<svec->size; i++) {
        printf("- %3u: %p\n", i, (void*)(svec->vector[i]));
    }
}

static int _sortedVectorAdd(SortedVector svec, void* elm) {
    if (!svec || !elm) return 1;
    if (_sortedVectorInit(svec)) return 2;
    if (_sortedVectorDuplicateCapacityIfNeeded(svec)) return 3;

    uint32_t idx = _sortedVectorGetElementIdxLogRight(svec, elm);
    if (idx==(uint32_t)(-1)) {
        fprintf(stderr, "_sortedVectorAdd: idx=-1??\n");
        return 4;
    }

    if (svec->size>idx) memmove(svec->vector+idx+1, svec->vector+idx, sizeof(void*)*(svec->size-idx));
    (svec->vector)[idx] = elm;
    (svec->size)++;
    
    return 0;
}

static void _sortedVectorDestroyElement(SortedVector svec, void* elm) {
    if (!svec || !elm) return;
    if (svec->destroy) (svec->destroy)(elm);
}

static int _sortedVectorDeleteAt(SortedVector svec, uint32_t idx) {
    if (_sortedVectorInit(svec) || idx>=svec->size || idx==(uint32_t)(-1)) return 1;
    _sortedVectorDestroyElement(svec, (svec->vector)[idx]);
    (svec->size)--;
    if (svec->size>idx) memmove(svec->vector+idx, svec->vector+idx+1, sizeof(void*)*(svec->size-idx));
    _sortedVectorHalveCapacityIfNeeded(svec);
    return 0;
}

static int _sortedVectorDeleteElement(SortedVector svec, void* elm) {
    if (_sortedVectorInit(svec)) return 1;
    uint32_t idx = _sortedVectorGetElementIdx(svec, elm);
    
    if (idx<(uint32_t)(-1)) {
        return _sortedVectorDeleteAt(svec, idx);
    }    
    return 0;
}

static int _sortedVectorDeleteAll(SortedVector svec) {
    if (!svec) return 1;
    if (!(svec->vector) || !(svec->size)) return 0;

    uint32_t s=svec->size;
    if (svec->vector && s && svec->destroy) {
        for (uint32_t i=0; i<s; i++) (svec->destroy)((svec->vector)[i]);
    }
    svec->size = 0;
    if (svec->capacity<=DEFAULT_SIZE) return 0;

    uint32_t cap=DEFAULT_SIZE;
    void** arr = realloc(svec->vector, cap*sizeof(void*));
    if (!arr) return 0;
    svec->vector = arr;
    svec->capacity = cap;
    return 0;
}

static int _sortedVectorSort(SortedVector svec) {
    if (!svec) return 1;
    if (!(svec->vector) || !(svec->size)) return 0;
    qsort(svec->vector, svec->size, sizeof(void*), svec->compare);
    return 0;
}

static int _sortedVectorCheckIfUnsorted(SortedVector svec) {
    if (!svec) return 1;
    if (!(svec->vector) || !(svec->size)) return 0;
    uint32_t n=svec->size-1;
    for (uint32_t i=0; i<n; i++) {
        if ((svec->compare)(svec->vector+i, svec->vector+i+1)>0) return 1;
    }
    return 0;
}

static int _sortedVectorShrinkToFit(SortedVector svec) {
    if (_sortedVectorInit(svec) || svec->size>=svec->capacity) return 1;
    if (svec->capacity<=DEFAULT_SIZE || !(svec->vector) || svec->size==svec->capacity) return 0;
    uint32_t ncap = (svec->size>DEFAULT_SIZE)?svec->size:DEFAULT_SIZE;
    void** arr = realloc(svec->vector, ncap*sizeof(void*));
    if (!arr) return -1;
    svec->vector = arr;
    svec->capacity = ncap;
    return 0;
}


static int _sortedVectorsMerge(void** dest, void** a, uint32_t sa, void** b, uint32_t sb, CompareFunc compare) {
    if (!dest || !compare) return 1;

    uint32_t id=0, sd=sa+sb, ia=0, ib=0;
    for (; id<sd; id++) {
        if (ia>=sa) dest[id]=b[ib++];
        else if (ib>=sb) dest[id]=a[ia++];
        else if (compare(a+ia, b+ib)>0) dest[id]=b[ib++];
        else dest[id]=a[ia++];
    }
    return 0;
}

static int _sortedVectorMergeT0(SortedVector destSrc1, SortedVector src2) {
    fprintf(stderr, "`_sortedVectorMergeT0`: *may* contain NULL bytes\n");
    if (!destSrc1 || !src2 || destSrc1==src2 || destSrc1->compare!=src2->compare) return 1;
    if (destSrc1->size+src2->size<=0) return 0;

    void** newArr = malloc((destSrc1->size+src2->size)*sizeof(void*));
    if (!newArr) return 1;

    int res = _sortedVectorsMerge(newArr, destSrc1->vector, destSrc1->size, src2->vector, src2->size, destSrc1->compare);
    if (res) {
        free(newArr);
        return res;
    }

    destSrc1->size+=src2->size;
    destSrc1->capacity = destSrc1->size;
    free(destSrc1->vector);
    destSrc1->vector = newArr;

    return 0;
}

static int _sortedVectorCopyElementsFromVector(SortedVector dest, SortedVector src) {
    fprintf(stderr, "`_sortedVectorCopyElementsFromVector`: *may* contain NULL bytes\n");

    if (!dest || !src || dest->compare!=src->compare || dest==src) return 1;
    _sortedVectorDeleteAll(dest);
    if (dest->size) return 1;
    if (!(src->size)) return 0;

    uint32_t ncap = src->size>DEFAULT_SIZE?src->size:DEFAULT_SIZE;

    if (ncap!=dest->capacity) {
        void** arr = realloc(dest->vector, ncap*sizeof(void*));
        if (!arr) return 1;
        dest->vector = arr;
    }
    dest->capacity = ncap;
    dest->size = src->size;

    memcpy(dest->vector, src->vector, (src->size)*sizeof(void*));
    return 0;
}

static int _min(int a, int b) {
    return (a>b)?b:a;
}

static int _intCompareFunc(const void* a, const void* b) {
    if (!a && !b) return 0;
    if (!a || !b) return a?-1:1;
    int na=*(int*)a, nb = *(int*)b;
    if (na>nb) return 1;
    if (na<nb) return -1;
    return 0;
}

static int _sortedVectorDeleteFromVector(SortedVector real, SortedVector helper) {
    if (!real) return 1;
    if (!helper || !(helper->size) || !(real->size)) return 0;
    if (real->compare!=helper->compare) return 1;
    
    uint32_t n2=helper->size, n1=real->size;
    uint32_t num = _min(n1, n2);
    if (!num) return 0;
    int* indexes = malloc(num*sizeof(int));
    if (!indexes) return 1;

    uint32_t arridx = 0;
    for (uint32_t i=0; i<n2; i++) {
        int idx = _sortedVectorFindElement(real, (helper->vector)[i]);
        if (idx>=0) indexes[arridx++] = idx;
    }
    
    if (arridx<=0) {
        free(indexes);
        return 0;
    }

    num = arridx;
    qsort(indexes, num, sizeof(int), _intCompareFunc);
    for (uint32_t i=0; i<num; i++) _sortedVectorDestroyElement(real, (real->vector)[indexes[i]]);

    uint32_t read=0, write=0, delete=0;
    while (read<n1) {
        if (delete<num && indexes[delete]==(int)read) {
            delete++;
            read++;
            continue;
        }
        (real->vector)[write++] = (real->vector)[read++];
    }
    real->size -= num;
    free(indexes);
    _sortedVectorShrinkToFit(real);

    return 0;
}

int sortedVectorAdd(SortedVector svec, void* elm) {   // Do not add something that you've already added before -> UB
    return _sortedVectorAdd(svec, elm);
}

int sortedVectorDeleteElement(SortedVector svec, void* elm) {
    return _sortedVectorDeleteElement(svec, elm);
}

int sortedVectorDeleteElementAt(SortedVector svec, uint32_t idx) {
    return _sortedVectorDeleteAt(svec, idx);
}

int sortedVectorFindElement(SortedVector svec, const void* elm) {
    return _sortedVectorFindElement(svec, elm);
}

const void* sortedVectorGetElementAt(SortedVector svec, uint32_t idx) {
    return _sortedVectorGetElementAt(svec, idx);
}

int sortedVectorSort(SortedVector svec) {
    if (_sortedVectorCheckIfUnsorted(svec)) return _sortedVectorSort(svec);
    else return 0;
}

int sortedVectorDeleteAllElements(SortedVector svec) {
    return _sortedVectorDeleteAll(svec);
}

int sortedVectorShrinkToFit(SortedVector svec) {
    return _sortedVectorShrinkToFit(svec);
}

int sortedVectorMerge(SortedVector destSrc1, SortedVector src2) {
    return _sortedVectorMergeT0(destSrc1, src2);
}

DestroyFunc sortedVectorSetDestroyFunc(SortedVector svec, DestroyFunc dfunc) {
    if (!svec) return NULL;
    DestroyFunc ret = svec->destroy;
    svec->destroy = dfunc;
    return ret;
}

int sortedVectorDestroy(SortedVector svec) {
    if (!svec) return 1;
    uint32_t s = svec->size;
    if (svec->vector && s && svec->destroy) {
        for (uint32_t i=0; i<s; i++) (svec->destroy)((svec->vector)[i]);
    }
    svec->size = 0;
    if (svec->vector) free(svec->vector);
    svec->vector = NULL;
    svec->capacity = 0;
    free(svec);

    deletions++;
    return 0;
}

int sortedVectorCopyElementsFromVector(SortedVector dest, SortedVector src) {
    return _sortedVectorCopyElementsFromVector(dest, src);
}

int sortedVectorDeleteFromVector(SortedVector real, SortedVector helper) {
    return _sortedVectorDeleteFromVector(real, helper);
}

void sortedVectorPrintDebug() {
    printf("SortedVector:  %6d creations,  %6d deletions\n", creations, deletions);
}