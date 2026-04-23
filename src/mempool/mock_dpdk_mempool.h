#ifndef MOCK_DPDK_MEMPOOL_H
#define MOCK_DPDK_MEMPOOL_H

#include <cstdlib>
#include <cstring>
#include "mempool.h"

/**
 * @brief Mock DPDK 内存池 —— 用于无 DPDK 环境下的测试
 *
 * 模拟 rte_malloc / rte_free 的行为：
 * - Alloc 调用 std::aligned_alloc 分配对齐内存
 * - Free 调用 std::free 释放
 * - 支持 MemType/size/align 参数验证
 *
 * 用法：与 MempoolManager 交替传入 SkipList::SetMempool()，
 * 验证 SkipList 在不同内存池实现间的无缝切换能力。
 */
class MockDpdkMempool : public IMempool
{
private:
    size_t mTotalAlloc;   // 总分配字节数（统计用）
    size_t mAllocCount;   // 分配次数
    size_t mFreeCount;    // 释放次数

public:
    MockDpdkMempool()
        : mTotalAlloc(0), mAllocCount(0), mFreeCount(0)
    {
    }

    ~MockDpdkMempool() override = default;

    MockDpdkMempool(const MockDpdkMempool&) = delete;
    MockDpdkMempool& operator=(const MockDpdkMempool&) = delete;

    void* Alloc(MemType type, size_t size, unsigned align) override
    {
        (void)type;

        if (align == 0)
        {
            align = 1;
        }

        void *ptr = std::aligned_alloc(align, size);
        if (ptr)
        {
            mTotalAlloc += size;
            ++mAllocCount;
        }
        return ptr;
    }

    void Free(void *ptr) override
    {
        if (ptr)
        {
            std::free(ptr);
            ++mFreeCount;
        }
    }

    size_t TotalAlloc() const { return mTotalAlloc; }
    size_t AllocCount() const { return mAllocCount; }
    size_t FreeCount() const { return mFreeCount; }
    size_t ActiveCount() const { return mAllocCount - mFreeCount; }
};

#endif /* MOCK_DPDK_MEMPOOL_H */
