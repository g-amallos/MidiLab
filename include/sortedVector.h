#ifndef SORTED_VECTOR_H
#define SORTED_VECTOR_H


typedef int (*CompareFunc)(const void* a, const void* b);
typedef void (*DestroyFunc)(void* a);
typedef void (*ApplyFunc)(void* a);
typedef struct sorted_vector* SortedVector;



SortedVector sortedVectorCreate(CompareFunc compare, DestroyFunc destroy);
SortedVector sortedVectorCreateFromData(CompareFunc compare, DestroyFunc destroy, uint32_t num, void** data);
uint32_t sortedVectorGetSize(SortedVector svec);
uint32_t sortedVectorGetCapacity(SortedVector svec);
uint32_t sortedVectorGetFreeSpace(SortedVector svec);
const void* sortedVectorGetElementAt(SortedVector svec, uint32_t idx);
const void** sortedVectorGetElements(SortedVector svec);
int sortedVectorSort(SortedVector svec);
int sortedVectorAdd(SortedVector svec, void* elm);                        // Do not add something that you've already added before -> UB
int sortedVectorDeleteElement(SortedVector svec, void* elm);
int sortedVectorDeleteElementAt(SortedVector svec, uint32_t idx);
int sortedVectorDeleteAllElements(SortedVector svec);
int sortedVectorFindElement(SortedVector svec, const void* elm);
int sortedVectorShrinkToFit(SortedVector svec);


int sortedVectorMerge(SortedVector destSrc1, SortedVector src2);
int sortedVectorCopyElementsFromVector(SortedVector dest, SortedVector src);    // Deletes any existing contents of the destination
int sortedVectorDeleteFromVector(SortedVector real, SortedVector helper);       // Deletes every element of real that is in helper. If DestroyFunc is not NULL, the elements of helper shouldn't be used again.

DestroyFunc sortedVectorSetDestroyFunc(SortedVector svec, DestroyFunc dfunc);   // Returns the old DestroyFunc and sets the new
int sortedVectorDestroy(SortedVector svec);


void sortedVectorPrintDebug();


#endif