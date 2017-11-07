/**
 * @file
 */
/******************************************************************************
* Copyright 2013, Qualcomm Connected Experiences, Inc.
*
******************************************************************************/

#include "aj_target.h"
#include "aj_util.h"
//#include <qcom/qcom_mem.h>
#include "aj_malloc.h"

static const AJ_HeapConfig heapConfig[] = {
    { 8,    4, 1 },
    { 16,   4, 1 },
    { 32,   2, 1 },
    { 64,   2, 1 },
    { 128,  2, 1 },
    { 256,  2, 1 },
    { 384,  2, 1 },
    { 512,  2, 0 },
    { 4096, 1, 0 }
};
#define AJ_HEAP_WORD_COUNT (7172 / 4)
static uint32_t heap[AJ_HEAP_WORD_COUNT];

void AJ_PoolAllocInit(void)
{
    //prepare the heap
    size_t heapSz;

    heapSz = AJ_PoolRequired(heapConfig, ArraySize(heapConfig));
    if (heapSz > sizeof(heap)) {
        AJ_Printf("Heap space is too small %d required %d\n", sizeof(heap), heapSz);
        return;
    }
    AJ_PoolInit(heap, heapSz, heapConfig, ArraySize(heapConfig));
    AJ_PoolDump();
}

void* AJ_Malloc(uint32_t sz)
{
    void* mem = AJ_PoolAlloc(sz);
    if (!mem) {
        AJ_Printf("!!!Failed to malloc %d bytes\n", sz);
    }
    return mem;
}
void AJ_Free(void* mem)
{
    if ((mem > (void*)&heap) && (mem < (void*)&heap[AJ_HEAP_WORD_COUNT])) {
        AJ_PoolFree(mem);
    }
}

void* AJ_Realloc(void* ptr, uint32_t size)
{
    return AJ_PoolRealloc(ptr, size);
}

