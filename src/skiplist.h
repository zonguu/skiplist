#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <vector>
#include <iostream>
#include <string>
#include <assert.h>

#include "stdint.h"

#include "list.h"
#include "list_template.h"
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
    struct list_head list[N];
    ListNode(const K &k, const V &v) : key(k), value(v)
    {
        // list_init(&list);
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
    list_head           head[H];
    IMempool           *mPool;

public:
    SkipList();
    ~SkipList();

    int32_t Put(const K &key, const V &value);

    void findGENode(const K & key, list_head* searchRes[H], GOECompareRes &res);
    ListNode<K, V, H>* findNode(const K &key, GOECompareRes &res);

    template<size_t h>
    void DisplaySpecifiedHeight()
    {
        std::cout << "display height " << h << std::endl;
        template_list_for_each_entry_array<ListNode<K, V, H>, list_head, h, H>(&head[h], &ListNode<K, V, H>::list, [](ListNode<K, V, H> *node)
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
        template_list_for_each_entry_array<ListNode<K, V, H>, list_head, h, H>(&head[h], &ListNode<K, V, H>::list, [&](ListNode<K, V, H> *node)
        {
            keys.push_back(node->key);
        });
        return keys;
    }

    const V* GetValue(const K &key);

    /**
     * @brief 利用跳表多层索引快速查找 key
     * @param key 要查找的键
     * @param value 输出参数，找到时写入对应的 value
     * @return SKIPLIST_OK 找到, SKIPLIST_ERR 未找到
     */
    int32_t Get(const K &key, V &value);

    /**
     * @brief 查找 key 是否存在（不返回 value）
     * @param key 要查找的键
     * @return true 存在, false 不存在
     */
    bool Find(const K &key);

    /**
     * @brief 删除指定 key 的节点
     * @param key 要删除的键
     * @return SKIPLIST_OK 删除成功, SKIPLIST_ERR 键不存在
     *
     * 删除流程：
     * 1. 从最高层向下查找，记录每一层中目标节点的前驱位置
     * 2. 确认目标节点存在（第0层 searchRes[0] 指向的节点 key 与目标相等）
     * 3. 从第0层到最高层，将该节点从各层链表中摘除
     * 4. 通过内存池释放节点内存
     */
    int32_t Delete(const K &key);

private:
    /**
     * @brief 通过内存池分配节点内存，并在已分配的内存上构造 ListNode
     * @param key   节点键
     * @param value 节点值
     * @return 构造完成的节点指针，内存池耗尽时返回 NULL
     *
     * 使用 placement new 在内存池分配的原始内存上构造对象，
     * 避免内存池的 slab 大小与 ListNode 实际大小不匹配的问题。
     */
    ListNode<K, V, H>* allocateNode(const K &key, const V &value);

    /**
     * @brief 析构节点对象并通过内存池释放内存
     * @param node 要释放的节点指针
     *
     * 显式调用析构函数后再将内存归还内存池，
     * 确保 std::string、std::vector 等非 POD 类型的正确清理。
     */
    void deallocateNode(ListNode<K, V, H> *node);

public:
    /**
     * @brief 设置内存池，必须在任何 Put 操作之前调用
     * @param pool 内存池指针，NULL 表示使用默认的 new/delete
     *
     * 默认情况下 SkipList 使用 new/delete 分配节点。
     * 传入 SlabPool 或 DpdkMempool 后，所有节点分配/释放都通过内存池进行。
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
     * 利用跳表第 0 层的有序链表，从 start 位置开始遍历到 end 位置。
     * 时间复杂度：O(log n + k)，k 为区间内元素数量。
     */
    std::vector<std::pair<K, V>> RangeQuery(const K &start, const K &end);

private:
};

#endif /* SKIPLIST_H */
