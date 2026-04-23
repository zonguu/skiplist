#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <cstddef>
#include <cstdint>

/**
 * @brief 内存类型枚举
 *
 * 每种需要内存池管理的对象类型对应一个枚举值。
 * 新增类型时在此扩展，并在 MempoolManager 中注册对应的 slab 配置。
 */
enum class MemType : uint32_t
{
    SKIPLIST_NODE = 0,   // 跳表节点
    MAX_TYPE
};

/**
 * @brief 内存池抽象接口，设计目标兼容 DPDK rte_malloc / rte_free 风格
 *
 * DPDK 内存分配接口签名：
 *   void *rte_malloc(const char *type, size_t size, unsigned align);
 *   void  rte_free(void *ptr);
 *
 * 本接口采用 MemType 枚举替代字符串 type，避免运行时字符串比较，
 * 同时保持与 DPDK 的映射能力（MemType → type_name 表）。
 */
class IMempool
{
public:
    virtual ~IMempool() = default;

    /**
     * @brief 分配内存
     * @param type  内存类型枚举，决定从哪个 slab 池分配
     * @param size  请求分配的字节数
     * @param align 对齐要求（必须是 2 的幂）
     * @return 分配成功的内存指针，失败返回 NULL
     */
    virtual void* Alloc(MemType type, size_t size, unsigned align) = 0;

    /**
     * @brief 释放内存
     * @param ptr Alloc 返回的指针，NULL 为安全空操作
     */
    virtual void Free(void *ptr) = 0;
};

#endif /* MEMPOOL_H */
