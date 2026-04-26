#include <cstdint>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <vector>

#include "skiplist.h"
#include "list_template_atomic.h"
#include "likely/likely.h"

bool debugSwitch = false;

/* ============================================================ */
/* 键比较：LESS/GREATER/EQUAL                                   */
/* ============================================================ */
template <typename K>
static inline bool GOE(const K &a, const K &b, GOECompareRes &res)
{
    if (a < b)
    {
        res = GOECompareRes::LESS;
        return false;
    }
    else if (b < a)
    {
        res = GOECompareRes::GREATER;
    }
    else
    {
        res = GOECompareRes::EQUAL;
    }
    Printf("GOE res:%d\n", res);
    return true;
}

/* ============================================================ */
/* SkipList 构造 / 析构                                          */
/* ============================================================ */
template <typename K, typename V, size_t H>
SkipList<K, V, H>::SkipList() : mHeight(H), mRng(std::random_device{}()), mPool(NULL)
{
    for (size_t i = 0; i < H; ++i)
    {
        atomic_list_init(&head[i]);
    }
}

template <typename K, typename V, size_t H>
SkipList<K, V, H>::~SkipList()
{
    // 1. 释放延迟释放队列中的 tombstone 节点
    for (auto *node : mPendingDelete)
    {
        deallocateNode(node);
    }

    // 2. 释放链表中剩余的活跃节点
    atomic_template_list_for_each_entry_array_safe<ListNode<K, V, H>, atomic_list_head, 0, H>(&head[0], &ListNode<K, V, H>::list, [this](ListNode<K, V, H> *node)
    {
        this->deallocateNode(node);
    });
}

/* ============================================================ */
/* 节点分配 / 释放（通过内存池或 new/delete）                      */
/* ============================================================ */
template <typename K, typename V, size_t H>
ListNode<K, V, H>* SkipList<K, V, H>::allocateNode(const K &key, const V &value)
{
    void *mem = NULL;
    if (mPool)
    {
        mem = mPool->Alloc(MemType::SKIPLIST_NODE, sizeof(ListNode<K, V, H>), alignof(ListNode<K, V, H>));
    }
    else
    {
        mem = std::aligned_alloc(alignof(ListNode<K, V, H>), sizeof(ListNode<K, V, H>));
    }

    if (unlikely(mem == NULL))
    {
        return NULL;
    }

    // placement new：在已分配的内存上构造对象
    return new (mem) ListNode<K, V, H>(key, value);
}

template <typename K, typename V, size_t H>
void SkipList<K, V, H>::deallocateNode(ListNode<K, V, H> *node)
{
    if (unlikely(node == NULL))
    {
        return;
    }

    // 显式调用析构函数，清理 std::string / std::vector 等资源
    node->~ListNode<K, V, H>();

    if (mPool)
    {
        mPool->Free(node);
    }
    else
    {
        std::free(node);
    }
}

/* ============================================================ */
/* 核心：从最高层向下查找第一个 >= key 的节点                     */
/* 线程安全：读操作无锁，使用 acquire 语义遍历 atomic 链表指针    */
/* ============================================================ */
template <typename K, typename V, size_t N>
void SkipList<K, V, N>::findGENode(const K & key, atomic_list_head* searchRes[N], GOECompareRes &res)
{
    static const size_t kBaseOffset = atomic_template_offsetof(&ListNode<K, V, N>::list);

    atomic_list_head *pos = NULL;
    int height = N - 1;
    for (; height >= 0; --height)
    {
        Printf("find height %d\n", height);
        const size_t offset = kBaseOffset + height * sizeof(atomic_list_head);

        // 无锁遍历：用 acquire 读取 next 指针，与写线程的 release 形成 happens-before
        for (pos = head[height].next.load(std::memory_order_acquire);
             pos != &head[height];
             pos = pos->next.load(std::memory_order_acquire))
        {
            auto *entry = reinterpret_cast<ListNode<K, V, N>*>(reinterpret_cast<char*>(pos) - offset);

            // 跳过 tombstone 节点（已标记删除但未物理释放）
            if (entry->isDeleted.load(std::memory_order_acquire))
            {
                continue;
            }

            if (GOE<K>(entry->key, key, res))
            {
                break;
            }
        }
        searchRes[height] = pos;
    }
}

/* ============================================================ */
/* 公共查询：返回找到的节点指针（NULL 表示不存在）                 */
/* ============================================================ */
template <typename K, typename V, size_t H>
ListNode<K, V, H>* SkipList<K, V, H>::findNode(const K &key, GOECompareRes &res)
{
    atomic_list_head *searchRes[H];
    findGENode(key, searchRes, res);
    if (searchRes[0] != &head[0] && res == GOECompareRes::EQUAL)
    {
        return atomic_template_list_entry(searchRes[0], &ListNode<K, V, H>::list);
    }
    return NULL;
}

/* ============================================================ */
/* Put：插入或更新（单写者，内部持锁）                            */
/* ============================================================ */
template <typename K, typename V, size_t H>
int32_t SkipList<K, V, H>::Put(const K &key, const V &value)
{
    std::lock_guard<std::mutex> lock(mWriteMutex);

    GOECompareRes res = GOECompareRes::GREATER;
    atomic_list_head *searchRes[H];
    findGENode(key, searchRes, res);
    Printf("Put res %d\n", res);

    if (searchRes[0] != &head[0] && res == GOECompareRes::EQUAL)
    {
        auto *same = atomic_template_list_entry(searchRes[0], &ListNode<K, V, H>::list);
        same->UpdateValue(value);
    }
    else
    {
        ListNode<K, V, H> *node = allocateNode(key, value);
        if (unlikely(node == NULL))
        {
            return SKIPLIST_NO_MEM;
        }

        std::uniform_int_distribution<int> dist(1, mHeight);
        auto desHeight = dist(mRng);
        Printf("desHeight %u\n", desHeight);

        // 先初始化新节点各层指针（此时节点尚未连入链表，读者不可见）
        for (int i = 0; i < mHeight; ++i)
        {
            if (i < desHeight)
            {
                Printf("insert height %u\n", i);
                // 从高层到低层逐个 release 发布，保证读者看到时节点已完全构造
                atomic_list_add_prev(&node->list[i], searchRes[i]);
            }
            else
            {
                atomic_list_init(&node->list[i]);
            }
        }
    }
    Printf("GOECompareRes %d\n", res);
    return SKIPLIST_OK;
}

/* ============================================================ */
/* 查询接口（无锁，可多线程并发）                                  */
/* ============================================================ */
template <typename K, typename V, size_t H>
int32_t SkipList<K, V, H>::Get(const K &key, V &value)
{
    GOECompareRes res;
    auto *node = findNode(key, res);
    if (node)
    {
        value = node->value;
        return SKIPLIST_OK;
    }
    return SKIPLIST_ERR;
}

template <typename K, typename V, size_t H>
bool SkipList<K, V, H>::Find(const K &key)
{
    GOECompareRes res;
    return findNode(key, res) != NULL;
}

/* ============================================================ */
/* RangeQuery：范围查询 [start, end]（无锁，可多线程并发）         */
/* ============================================================ */
template <typename K, typename V, size_t H>
std::vector<std::pair<K, V>> SkipList<K, V, H>::RangeQuery(const K &start, const K &end)
{
    std::vector<std::pair<K, V>> result;

    // 先找到 start 的位置
    GOECompareRes res;
    atomic_list_head *searchRes[H];
    findGENode(start, searchRes, res);

    atomic_list_head *pos = searchRes[0];
    const size_t offset = atomic_template_offsetof(&ListNode<K, V, H>::list);

    // 从 start 位置开始遍历第 0 层链表
    for (; pos != &head[0]; pos = pos->next.load(std::memory_order_acquire))
    {
        auto *entry = reinterpret_cast<ListNode<K, V, H>*>(reinterpret_cast<char*>(pos) - offset);

        // 跳过小于 start 的节点（当 start 不存在时，pos 指向第一个 >= start 的节点）
        if (entry->key < start)
        {
            continue;
        }

        // 跳过 tombstone 节点
        if (entry->isDeleted.load(std::memory_order_acquire))
        {
            continue;
        }

        // 超过 end 范围，停止遍历
        if (end < entry->key)
        {
            break;
        }

        result.emplace_back(entry->key, entry->value);
    }

    return result;
}

/* ============================================================ */
/* Delete：标记删除 + 物理摘除 + 延迟释放（单写者，内部持锁）      */
/* ============================================================ */
template <typename K, typename V, size_t H>
int32_t SkipList<K, V, H>::Delete(const K &key)
{
    std::lock_guard<std::mutex> lock(mWriteMutex);

    GOECompareRes res = GOECompareRes::GREATER;
    atomic_list_head *searchRes[H];
    findGENode(key, searchRes, res);

    // 目标节点必须存在：第0层 searchRes[0] 不是头节点，且 key 相等
    if (searchRes[0] == &head[0] || res != GOECompareRes::EQUAL)
    {
        return SKIPLIST_ERR;    // 键不存在
    }

    // 通过第0层的 list 指针反解出节点地址
    auto *target = atomic_template_list_entry(searchRes[0], &ListNode<K, V, H>::list);

    // 1. 先标记 tombstone（读者从此不再返回此 key）
    target->isDeleted.store(true, std::memory_order_release);

    // 2. 从每一层链表中物理摘除（减少后续读遍历开销）
    for (int i = 0; i < mHeight; ++i)
    {
        // 如果该层中 target 的 list[i] 已经初始化（即确实插入了该层）
        if (target->list[i].next.load(std::memory_order_acquire) != &target->list[i])
        {
            atomic_list_del(&target->list[i]);
        }
    }

    // 3. 节点内存不立即释放（可能有读线程正在遍历），放入延迟释放队列
    mPendingDelete.push_back(target);

    return SKIPLIST_OK;
}

/* ============================================================ */
/* Compact：重建跳表，清理 tombstone 节点，释放内存               */
/* ============================================================ */
template <typename K, typename V, size_t H>
void SkipList<K, V, H>::Compact()
{
    std::lock_guard<std::mutex> lock(mWriteMutex);

    // 1. 释放所有 pendingDelete 中的节点
    for (auto *node : mPendingDelete)
    {
        deallocateNode(node);
    }
    mPendingDelete.clear();

    // 2. 遍历第 0 层，收集所有存活节点
    std::vector<ListNode<K, V, H>*> aliveNodes;
    const size_t offset = atomic_template_offsetof(&ListNode<K, V, H>::list);

    for (atomic_list_head *pos = head[0].next.load(std::memory_order_acquire);
         pos != &head[0];
         pos = pos->next.load(std::memory_order_acquire))
    {
        auto *entry = reinterpret_cast<ListNode<K, V, H>*>(reinterpret_cast<char*>(pos) - offset);
        if (!entry->isDeleted.load(std::memory_order_acquire))
        {
            aliveNodes.push_back(entry);
        }
    }

    // 3. 重新初始化各层链表头
    for (int i = 0; i < mHeight; ++i)
    {
        atomic_list_init(&head[i]);
    }

    // 4. 重新插入所有存活节点
    for (auto *node : aliveNodes)
    {
        node->isDeleted.store(false, std::memory_order_relaxed);

        // 为每个节点重新生成随机高度
        std::uniform_int_distribution<int> dist(1, mHeight);
        auto desHeight = dist(mRng);

        for (int i = 0; i < mHeight; ++i)
        {
            if (i < desHeight)
            {
                atomic_list_add_tail(&node->list[i], &head[i]);
            }
            else
            {
                atomic_list_init(&node->list[i]);
            }
        }
    }
}

/* ============================================================ */
/* 显式实例化                                                    */
/* ============================================================ */
template class SkipList<int, int, 8>;
template class SkipList<std::vector<int>, int, 8>;
template class SkipList<std::string, int, 8>;
