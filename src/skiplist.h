#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <vector>
#include <iostream>
#include <string>
#include <assert.h>
#include <mutex>
#include <atomic>
#include <utility>
#include <type_traits>
#include <iterator>

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

/* ============================================================ */
/* SkipList 前向迭代器                                           */
/* ============================================================ */
template <typename K, typename V, size_t N, bool IsConst>
class SkipListIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<K, V>;
    using difference_type = std::ptrdiff_t;
    using reference = std::conditional_t<IsConst,
        std::pair<const K&, const V&>,
        std::pair<const K&, V&>>;
    using pointer = void;

private:
    using Node = ListNode<K, V, N>;
    using NodePtr = std::conditional_t<IsConst, const Node*, Node*>;

    NodePtr mNode;                       // nullptr 表示 end
    const atomic_list_head* mHead;       // 第 0 层头节点，用于判断终点
    size_t mOffset;                      // list[0] 的 offsetof

    void advance()
    {
        if (!mNode) return;
        do {
            atomic_list_head* next = mNode->list[0].next.load(std::memory_order_acquire);
            if (next == mHead) {
                mNode = nullptr;
                return;
            }
            mNode = reinterpret_cast<NodePtr>(reinterpret_cast<char*>(next) - mOffset);
        } while (mNode->isDeleted.load(std::memory_order_acquire));
    }

public:
    SkipListIterator() : mNode(nullptr), mHead(nullptr), mOffset(0) {}

    SkipListIterator(NodePtr node, const atomic_list_head* head, size_t offset)
        : mNode(node), mHead(head), mOffset(offset)
    {
        // 如果起始节点是 tombstone，先前进到第一个有效节点
        if (mNode && mNode->isDeleted.load(std::memory_order_acquire)) {
            advance();
        }
    }

    // 允许 iterator → const_iterator 隐式转换
    template<bool WasConst, typename = std::enable_if_t<IsConst && !WasConst>>
    SkipListIterator(const SkipListIterator<K, V, N, WasConst>& other)
        : mNode(other.mNode), mHead(other.mHead), mOffset(other.mOffset) {}

    reference operator*() const
    {
        return reference{mNode->key, mNode->value};
    }

    // proxy pointer for operator->
    struct ProxyPtr {
        reference ref;
        reference* operator->() { return &ref; }
    };

    ProxyPtr operator->() const
    {
        return ProxyPtr{reference{mNode->key, mNode->value}};
    }

    SkipListIterator& operator++()
    {
        advance();
        return *this;
    }

    SkipListIterator operator++(int)
    {
        SkipListIterator tmp = *this;
        advance();
        return tmp;
    }

    bool operator==(const SkipListIterator& other) const
    {
        return mNode == other.mNode;
    }

    bool operator!=(const SkipListIterator& other) const
    {
        return !(*this == other);
    }

    // 暴露底层节点指针（仅供 SkipList 内部或高级使用）
    NodePtr node() const { return mNode; }
};

/* ============================================================ */
/* SkipList 主类                                                 */
/* ============================================================ */
template <typename K, typename V, size_t H>
class SkipList
{
    static_assert(H >= 1, "SkipList height H must be >= 1");

public:
    using iterator = SkipListIterator<K, V, H, false>;
    using const_iterator = SkipListIterator<K, V, H, true>;

private:
    int32_t             mHeight;
    std::mt19937        mRng;
    atomic_list_head    head[H];
    IMempool           *mPool;
    std::mutex          mWriteMutex;                     // 写操作互斥锁（单写者）
    std::vector<ListNode<K, V, H>*> mPendingDelete;      // 延迟释放队列（tombstone 节点）

    static size_t baseOffset()
    {
        return atomic_template_offsetof(&ListNode<K, V, H>::list);
    }

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

    /* -------------------------------------------------------- */
    /* 迭代器接口                                                */
    /* -------------------------------------------------------- */
    iterator begin()
    {
        const size_t offset = baseOffset();
        atomic_list_head* pos = head[0].next.load(std::memory_order_acquire);
        while (pos != &head[0]) {
            auto* node = reinterpret_cast<ListNode<K, V, H>*>(reinterpret_cast<char*>(pos) - offset);
            if (!node->isDeleted.load(std::memory_order_acquire)) {
                return iterator(node, &head[0], offset);
            }
            pos = pos->next.load(std::memory_order_acquire);
        }
        return iterator(nullptr, &head[0], offset);
    }

    iterator end()
    {
        return iterator(nullptr, &head[0], baseOffset());
    }

    const_iterator begin() const
    {
        const size_t offset = baseOffset();
        const atomic_list_head* pos = head[0].next.load(std::memory_order_acquire);
        while (pos != &head[0]) {
            auto* node = reinterpret_cast<const ListNode<K, V, H>*>(reinterpret_cast<const char*>(pos) - offset);
            if (!node->isDeleted.load(std::memory_order_acquire)) {
                return const_iterator(node, &head[0], offset);
            }
            pos = pos->next.load(std::memory_order_acquire);
        }
        return const_iterator(nullptr, &head[0], offset);
    }

    const_iterator end() const
    {
        return const_iterator(nullptr, &head[0], baseOffset());
    }

    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

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
