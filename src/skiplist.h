#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <vector>
#include <iostream>
#include <string>
#include <assert.h>
#include <mutex>
#include <atomic>

#include "stdint.h"

#include "list_atomic.h"
#include "list_template_atomic.h"
#include "mempool/mempool.h"
#include <random>

extern bool debugSwitch;

#define Printf(...) \
    do { if (debugSwitch) \
    printf(__VA_ARGS__); \
} while (0)

#define SKIPLIST_MAX_HEIGHT (8)

#define SKIPLIST_OK (0)
#define SKIPLIST_ERR (-1)
#define SKIPLIST_NO_MEM (-2)

inline std::ostream& operator<<(std::ostream& os, const std::vector<int>& vec)
{
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        if (i) os << ",";
        os << vec[i];
    }
    os << "]";
    return os;
}

template <typename K, typename V, size_t N>
struct ListNode
{
    K key;
    V value;
    struct atomic_list_head list[N];
    std::atomic<bool> isDeleted{false};   // 标记删除（tombstone）

    ListNode(const K &k, const V &v) : key(k), value(v)
    {
        // atomic_list_init(&list);
    }

    void Init(const K &k, const V &v)
    {
        key = k;
        value = v;
    }

    void UpdateKey(const K &k)
    {
        key = k;
    }

    void UpdateValue(const V &v)
    {
        value = v;
    }

    bool operator<(const ListNode<K, V, N> &other) const
    {
        return key < other.key;
    }
};

template <typename K, typename V, size_t N>
void FreeListNode(ListNode<K, V, N> *node)
{
    delete node;
}

enum GOECompareRes
{
    LESS = 0,
    EQUAL = 1,
    GREATER = 2,
};

template <typename K>
bool GOE(const K &a, const K &b, GOECompareRes &res);

template <typename K, typename V, size_t H>
class SkipList
{
    static_assert(H >= 1, "SkipList height H must be >= 1");

private:
    int32_t             mHeight;
    std::mt19937        mRng;
    atomic_list_head    head[H];
    IMempool           *mPool;
    std::mutex          mWriteMutex;                     // 写操作互斥锁（单写者）
    std::vector<ListNode<K, V, H>*> mPendingDelete;      // 延迟释放队列（tombstone 节点）

public:
    SkipList();
    ~SkipList();

    int32_t Put(const K &key, const V &value);

    void findGENode(const K & key, atomic_list_head* searchRes[H], GOECompareRes &res);
    ListNode<K, V, H>* findNode(const K &key, GOECompareRes &res);

    template<size_t h>
    void DisplaySpecifiedHeight()
    {
        std::cout << "display height " << h << std::endl;
        atomic_template_list_for_each_entry_array<ListNode<K, V, H>, atomic_list_head, h, H>(&head[h], &ListNode<K, V, H>::list, [](ListNode<K, V, H> *node)
                                                                { std::cout << node->key << "\t" << node->value << std::endl; });
    }

    template<size_t... Is>
    void DisplayAllHeightImpl(std::index_sequence<Is...>)
    {
        (DisplaySpecifiedHeight<Is>(), ...); // C++17折叠表达式
    }

    void DisplayAllHeight()
    {
        DisplayAllHeightImpl(std::make_index_sequence<H>{}); 
    }

    template<size_t h>
    std::vector<K> GetKeysAtHeight()
    {
        std::vector<K> keys;
        atomic_template_list_for_each_entry_array<ListNode<K, V, H>, atomic_list_head, h, H>(&head[h], &ListNode<K, V, H>::list, [&](ListNode<K, V, H> *node)
        {
            keys.push_back(node->key);
        });
        return keys;
    }

    /**
     * @brief 利用跳表多层索引快速查找 key
     * @param key 要查找的键
     * @param value 输出参数，找到时写入对应的 value
     * @return SKIPLIST_OK 找到, SKIPLIST_ERR 未找到
     *
     * 线程安全：读操作无锁，可多线程并发调用。
     */
    int32_t Get(const K &key, V &value);

    /**
     * @brief 查找 key 是否存在（不返回 value）
     * @param key 要查找的键
     * @return true 存在, false 不存在
     *
     * 线程安全：读操作无锁，可多线程并发调用。
     */
    bool Find(const K &key);

    /**
     * @brief 删除指定 key 的节点（tombstone 标记删除）
     * @param key 要删除的键
     * @return SKIPLIST_OK 删除成功, SKIPLIST_ERR 键不存在
     *
     * 线程安全：写操作内部持锁，需由调用方保证单写者。
     * 删除的节点内存不立即释放，放入 pendingDelete 队列，
     * 在 SkipList 析构时统一释放。如需即时回收内存，
     * 可调用 Compact() 重建跳表。
     */
    int32_t Delete(const K &key);

private:
    /**
     * @brief 通过内存池分配节点内存，并在已分配的内存上构造 ListNode
     */
    ListNode<K, V, H>* allocateNode(const K &key, const V &value);

    /**
     * @brief 析构节点对象并通过内存池释放内存
     */
    void deallocateNode(ListNode<K, V, H> *node);

public:
    /**
     * @brief 设置内存池，必须在任何 Put/Delete 操作之前调用
     * @param pool 内存池指针，NULL 表示使用默认的 new/delete
     *
     * 线程安全：非线程安全，必须在所有读写操作之前调用。
     */
    void SetMempool(IMempool *pool) { mPool = pool; }

    /**
     * @brief 获取当前绑定的内存池
     */
    IMempool* GetMempool() const { return mPool; }

    /**
     * @brief 范围查询：返回 [start, end] 区间内所有键值对（含边界）
     * @param start 范围起始键（包含）
     * @param end   范围结束键（包含）
     * @return 区间内的键值对列表，按 key 升序排列
     *
     * 线程安全：读操作无锁，可多线程并发调用。
     * 利用跳表第 0 层的有序链表，从 start 位置开始遍历到 end 位置。
     * 时间复杂度：O(log n + k)，k 为区间内元素数量。
     */
    std::vector<std::pair<K, V>> RangeQuery(const K &start, const K &end);

    /**
     * @brief 重建跳表，清理所有标记删除的节点，释放内存
     *
     * 线程安全：写操作内部持锁，需由调用方保证单写者，
     * 且调用期间无并发读者。
     */
    void Compact();

private:
};

#endif /* SKIPLIST_H */
