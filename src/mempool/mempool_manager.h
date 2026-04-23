#ifndef MEMPOOL_MANAGER_H
#define MEMPOOL_MANAGER_H

#include <array>
#include <cstddef>

#include "mempool.h"
#include "slab_pool.h"

/**
 * @brief 内存池配置：描述一种 MemType 对应的 slab 参数
 */
struct MemTypeConfig
{
    MemType     type;
    size_t      elemSize;
    size_t      capacity;
    unsigned    align;
};

/**
 * @brief 内存池管理器：统一管理多种类型的 SlabPool
 *
 * 初始化时根据配置表为每种 MemType 创建对应的 SlabPool。
 * Alloc 时根据 MemType 路由到对应的 SlabPool。
 *
 * 使用示例：
 *   MemTypeConfig configs[] = {
 *       { MemType::SKIPLIST_NODE, sizeof(ListNode<int,int,8>), 1000, 8 },
 *   };
 *   MempoolManager mgr(configs, 1);
 *   SkipList sl;
 *   sl.SetMempool(&mgr);
 */
class MempoolManager : public IMempool
{
private:
    std::array<SlabPool*, static_cast<size_t>(MemType::MAX_TYPE)> mPools;

public:
    /**
     * @brief 构造 MempoolManager
     * @param configs  配置数组，每个元素描述一种 MemType 的 slab 参数
     * @param count    配置数组元素个数
     */
    MempoolManager(const MemTypeConfig *configs, size_t count);

    ~MempoolManager();

    MempoolManager(const MempoolManager&) = delete;
    MempoolManager& operator=(const MempoolManager&) = delete;

    void* Alloc(MemType type, size_t size, unsigned align) override;
    void Free(void *ptr) override;

    SlabPool* GetPool(MemType type) const;
};

#endif /* MEMPOOL_MANAGER_H */
