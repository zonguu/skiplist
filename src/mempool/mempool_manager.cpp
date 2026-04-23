#include "mempool_manager.h"

MempoolManager::MempoolManager(const MemTypeConfig *configs, size_t count)
{
    mPools.fill(NULL);

    for (size_t i = 0; i < count; ++i)
    {
        uint32_t idx = static_cast<uint32_t>(configs[i].type);
        if (idx < static_cast<uint32_t>(MemType::MAX_TYPE))
        {
            mPools[idx] = new SlabPool(configs[i].elemSize, configs[i].capacity, configs[i].align);
        }
    }
}

MempoolManager::~MempoolManager()
{
    for (size_t i = 0; i < mPools.size(); ++i)
    {
        delete mPools[i];
        mPools[i] = NULL;
    }
}

void* MempoolManager::Alloc(MemType type, size_t size, unsigned align)
{
    (void)size;
    (void)align;

    uint32_t idx = static_cast<uint32_t>(type);
    if (idx >= static_cast<uint32_t>(MemType::MAX_TYPE))
    {
        return NULL;
    }

    SlabPool *pool = mPools[idx];
    if (pool == NULL)
    {
        return NULL;
    }

    return pool->Alloc();
}

void MempoolManager::Free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    // 遍历所有 pool，通过地址范围判断 ptr 属于哪个 SlabPool
    for (size_t i = 0; i < mPools.size(); ++i)
    {
        if (mPools[i] != NULL && mPools[i]->Contains(ptr))
        {
            mPools[i]->Free(ptr);
            return;
        }
    }
}

SlabPool* MempoolManager::GetPool(MemType type) const
{
    uint32_t idx = static_cast<uint32_t>(type);
    if (idx < static_cast<uint32_t>(MemType::MAX_TYPE))
    {
        return mPools[idx];
    }
    return NULL;
}
