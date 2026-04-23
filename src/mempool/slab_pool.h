#ifndef SLAB_POOL_H
#define SLAB_POOL_H

#include <cstddef>
#include <cstdint>
#include <new>

#include "list.h"

/**
 * @brief 单一 slab 内存池
 *
 * 管理一种固定大小的内存对象：
 * - 预分配一块大内存，切分为固定大小的 slab 单元
 * - 每个 slab 单元通过嵌入的 list_head 串联成空闲链表
 * - 分配时从空闲链表头部取出，释放时挂回链表头部
 *
 * 特点：O(1) 分配/释放，无系统调用开销，无外部碎片。
 */
class SlabPool
{
private:
    uint8_t    *mBase;          // 预分配的大内存起始地址
    size_t      mSlabSize;      // 每个 slab 单元大小（含对齐填充）
    size_t      mCapacity;      // 总 slab 数量
    size_t      mUsed;          // 已使用数量
    list_head   mFreeList;      // 空闲 slab 链表

public:
    /**
     * @brief 构造 SlabPool
     * @param elemSize  每个元素的实际大小
     * @param capacity  预分配的元素数量
     * @param align     对齐要求（必须是 2 的幂），默认 8 字节
     */
    SlabPool(size_t elemSize, size_t capacity, unsigned align = 8);

    ~SlabPool();

    SlabPool(const SlabPool&) = delete;
    SlabPool& operator=(const SlabPool&) = delete;

    void* Alloc();
    void Free(void *ptr);

    size_t Capacity() const { return mCapacity; }
    size_t Used() const { return mUsed; }
    size_t Available() const { return mCapacity - mUsed; }

    /**
     * @brief 判断 ptr 是否属于本 SlabPool 管理的地址范围
     */
    bool Contains(const void *ptr) const
    {
        if (ptr == NULL || mBase == NULL)
        {
            return false;
        }
        const uint8_t *p = static_cast<const uint8_t*>(ptr);
        return (p >= mBase) && (p < mBase + mSlabSize * mCapacity);
    }
};

#endif /* SLAB_POOL_H */
