#include <vector.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


struct vector {
    uint32_t size;
    uint32_t capacity;
    DestroyFunc destroy;
    void** elements;
};



Vector vectorCreate(DestroyFunc destroy) {  
    Vector vec = malloc(sizeof(struct vector));
    if (!vec) return NULL;
    vec->size = 0;
    vec->destroy = destroy;
    vec->capacity = 32;
    vec->elements = calloc(vec->capacity, sizeof(void*));
    if (!(vec->elements)) {
        free(vec);
        return NULL;
    }

    return vec;
}

static int _vecInit(Vector vec) {
    if (!vec) return 1;
    if (!(vec->elements)) {
        vec->size = 0;
        vec->capacity = 32;
        vec->elements = calloc(vec->capacity, sizeof(void*));
        if (!(vec->elements)) return 1;
    }
    return 0;
}

static int _vecDuplicateCapacityIfNeeded(Vector vec) {
    if (_vecInit(vec)) return 1;
    if (vec->size==vec->capacity) {
        uint32_t newCap = (vec->capacity<<1);
        void** new = realloc(vec->elements, sizeof(void*)*newCap);
        if (!new) return 1;
        vec->elements = new;
        vec->capacity = newCap;
    }
    return 0;
}

static int _vecHalveCapacityIfNeeded(Vector vec) {
    if (!vec || !(vec->elements)) return 1;
    if ((vec->size<<2)<vec->capacity) {
        uint32_t newCap = (vec->capacity<<1);
        void** new = realloc(vec->elements, sizeof(void*)*newCap);
        if (!new) return 1;
        vec->elements = new;
        vec->capacity = newCap;
    }
    return 0;
}

static int _vecAppend(Vector vec, void* elm) {
    if (!elm) return 1;
    if (_vecDuplicateCapacityIfNeeded(vec)) return 1;
    (vec->elements)[(vec->size)++] = elm;
    return 0;
}

static void _vecDestroyElement(Vector vec, void* elm) {
    if (!vec) return;
    if (vec->destroy) (vec->destroy)(elm);
}

static int _vecDeleteAt(Vector vec, int idx) {
    if (_vecInit(vec) || idx>=(int)vec->size || (int)vec->size+idx<0) return 1;
    idx = (idx>=0)?idx:((int)vec->size+idx);
    _vecDestroyElement(vec, (vec->elements)[idx]);
    (vec->size)--;
    if ((int)vec->size>idx) memmove(vec->elements+idx, vec->elements+idx+1, sizeof(void*)*((int)vec->size-idx));
    _vecHalveCapacityIfNeeded(vec);
    return 0;
}

static void* _vecGetElementAt(Vector vec, int idx) {
    if (_vecInit(vec) || idx>=(int)vec->size || (int)vec->size+idx<0) return NULL;
    idx = (idx>=0)?idx:((int)vec->size+idx);
    return (vec->elements)[idx];
}

static int _vecSetAt(Vector vec, int idx, void* elm) {
    if (_vecInit(vec) || idx>=(int)vec->size || (int)vec->size+idx<0) return 1;
    idx = (idx>=0)?idx:((int)vec->size+idx);
    _vecDestroyElement(vec, (vec->elements)[idx]);
    (vec->elements)[idx] = elm;
    return 0;
}

static int _vecDeleteElement(Vector vec, void* elm) {
    if (_vecInit(vec)) return 1;
    uint32_t n=vec->size, i=0;
    void** elements = vec->elements;
    while (i<n) {
        if (elements[i]==elm) {
            _vecDeleteAt(vec, i);
            n--;
        } else i++;
    }
    return 0;
}

static int _vecDeleteElements(Vector vec) {
    if (_vecInit(vec)) return 1;
    uint32_t s = vec->size;
    for (uint32_t i=0; i<s; i++) {
        _vecDestroyElement(vec, (vec->elements)[i]);
    }
    vec->size=0;
    uint32_t cap = 32;
    void** elm = realloc(vec->elements, cap*sizeof(void*));
    if (!elm) return 1;
    vec->elements = elm;
    vec->capacity = cap;    
    return 0;
}

int vectorAppend(Vector vec, void* element) {
    return _vecAppend(vec, element);
}

void* vectorGetAt(Vector vec, int idx) {
    return _vecGetElementAt(vec, idx);
}

int vectorDeleteAt(Vector vec, int idx) {
    return _vecDeleteAt(vec, idx);
}

uint32_t vectorGetSize(Vector vec) {
    if (!vec || !(vec->elements)) return 0;
    return vec->size;
}

int vectorSetAt(Vector vec, int idx, void* element) {
    return _vecSetAt(vec, idx, element);
}

int vectorDestroy(Vector vec) {
    if (!vec) return 1;
    uint32_t s = vec->size;
    if (vec->elements && s) {
        for (uint32_t i=0; i<s; i++) {
            _vecDestroyElement(vec, (vec->elements)[i]);
        }
    }
    if (vec->elements) free(vec->elements);
    vec->elements = NULL;
    free(vec);
    return 0;
}


int vectorApplyFunction(Vector vec, FindFunc find, ApplyFunc apply) {
    if (!vec || !find || !apply) return 1;
    if (!(vec->size) || !(vec->elements)) return 0;

    uint32_t n=vec->size;
    for (uint32_t i=0; i<n; i++) {
        if (find((vec->elements)[i])) apply((vec->elements)[i]);
    }
    return 0;
}

int vectorApplyFunctionForAllElements(Vector vec, ApplyFunc apply) {
    if (!vec || !apply) return 1;
    if (!(vec->size) || !(vec->elements)) return 0;
    uint32_t n=vec->size;
    for (uint32_t i=0; i<n; i++) apply((vec->elements)[i]);
    return 0;
}

int vectorDeleteElement(Vector vec, void* element) {
    return _vecDeleteElement(vec, element);
}

int vectorDeleteAllElements(Vector vec) {
    return _vecDeleteElements(vec);
}