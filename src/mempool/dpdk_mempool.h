#ifndef DPDK_MEMPOOL_H
#define DPDK_MEMPOOL_H

#include "mempool.h"

/**
 * @brief DPDK 内存池适配器
 *
 * 在已集成 DPDK 的环境中，直接透传 DPDK 的 rte_malloc / rte_free。
 * 编译时需要链接 DPDK 库（-lrte_eal）。
 *
 * 未集成 DPDK 时，本文件不参与编译，SlabPool 作为默认实现。
 */
class DpdkMempool : public IMempool
{
public:
    DpdkMempool() = default;
    ~DpdkMempool() override = default;

    // 禁止拷贝
    DpdkMempool(const DpdkMempool&) = delete;
    DpdkMempool& operator=(const DpdkMempool&) = delete;

    void* Alloc(const char *type, size_t size, unsigned align) override;
    void Free(void *ptr) override;
};

#endif /* DPDK_MEMPOOL_H */
