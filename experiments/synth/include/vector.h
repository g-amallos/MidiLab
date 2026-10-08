#ifndef VECTOR_H
#define VECTOR_H
#include <stdint.h>

typedef struct vector *Vector;
typedef void (*DestroyFunc)(void*);
typedef int (*FindFunc)(void*);
typedef void (*ApplyFunc)(void*);




Vector vectorCreate(DestroyFunc destroy);
int vectorAppend(Vector vec, void* element);
void* vectorGetAt(Vector vec, int idx);
int vectorDeleteAt(Vector vec, int idx);
uint32_t vectorGetSize(Vector vec);
int vectorSetAt(Vector vec, int idx, void* element);
int vectorDestroy(Vector vec);
int vectorApplyFunction(Vector vec, FindFunc find, ApplyFunc apply);
int vectorApplyFunctionForAllElements(Vector vec, ApplyFunc apply);
int vectorDeleteElement(Vector vec, void* element);
int vectorDeleteAllElements(Vector vec);

#endif