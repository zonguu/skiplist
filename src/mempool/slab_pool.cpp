#include <cstdlib>
#include <cstdio>
#include <algorithm>

#include "slab_pool.h"

static inline size_t AlignUp(size_t size, unsigned align)
{
    return (size + align - 1) & ~(align - 1);
}

SlabPool::SlabPool(size_t elemSize, size_t capacity, unsigned align)
    : mBase(NULL), mSlabSize(0), mCapacity(capacity), mUsed(0)
{
    if (align == 0 || (align & (align - 1)) != 0)
    {
        align = 8;
    }

    mSlabSize = AlignUp(elemSize, align);
    if (mSlabSize < sizeof(list_head))
    {
        mSlabSize = AlignUp(sizeof(list_head), align);
    }

    size_t totalBytes = mSlabSize * capacity;
    mBase = static_cast<uint8_t*>(std::aligned_alloc(align, totalBytes));
    if (mBase == NULL)
    {
        mCapacity = 0;
        return;
    }

    list_init(&mFreeList);

    for (size_t i = 0; i < capacity; ++i)
    {
        list_head *node = reinterpret_cast<list_head*>(mBase + i * mSlabSize);
        list_add_prev(node, &mFreeList);
    }
}

SlabPool::~SlabPool()
{
    if (mBase)
    {
        std::free(mBase);
        mBase = NULL;
    }
}

void* SlabPool::Alloc()
{
    if (mUsed >= mCapacity)
    {
        return NULL;
    }

    list_head *node = mFreeList.next;
    list_del(node);
    list_init(node);

    ++mUsed;
    return static_cast<void*>(node);
}

void SlabPool::Free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    list_head *node = static_cast<list_head*>(ptr);
    list_init(node);
    list_add_prev(node, &mFreeList);

    --mUsed;
}
