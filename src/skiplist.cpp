#include <cstdint>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <vector>

#include "skiplist.h"
#include "list_template.h"
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
/* 节点创建                                                      */
/* ============================================================ */
template <typename K, typename V, size_t N>
static inline ListNode<K, V, N>* createNewListNode(const K &key, const V &value)
{
    return new ListNode<K, V, N>(key, value);
}

/* ============================================================ */
/* SkipList 构造 / 析构                                          */
/* ============================================================ */
template <typename K, typename V, size_t H>
SkipList<K, V, H>::SkipList() : mHeight(H), mRng(std::random_device{}())
{
    for (size_t i = 0; i < H; ++i)
    {
        list_init(&head[i]);
    }
}

template <typename K, typename V, size_t H>
SkipList<K, V, H>::~SkipList()
{
    template_list_for_each_entry_array_safe<ListNode<K, V, H>, list_head, 0, H>(&head[0], &ListNode<K, V, H>::list, FreeListNode<K, V, H>);
}

/* ============================================================ */
/* 核心：从最高层向下查找第一个 >= key 的节点                     */
/* ============================================================ */
template <typename K, typename V, size_t N>
void SkipList<K, V, N>::findGENode(const K & key, list_head* searchRes[N], GOECompareRes &res)
{
    static const size_t kBaseOffset = template_offsetof(&ListNode<K, V, N>::list);

    list_head *pos = NULL;
    int height = N - 1;
    for (; height >= 0; --height)
    {
        Printf("find height %d\n", height);
        const size_t offset = kBaseOffset + height * sizeof(list_head);
        list_for_each(pos, &head[height])
        {
            auto *entry = reinterpret_cast<ListNode<K, V, N>*>(reinterpret_cast<char*>(pos) - offset);
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
    list_head *searchRes[H];
    findGENode(key, searchRes, res);
    if (searchRes[0] != &head[0] && res == GOECompareRes::EQUAL)
    {
        return template_list_entry(searchRes[0], &ListNode<K, V, H>::list);
    }
    return NULL;
}

/* ============================================================ */
/* Put：插入或更新                                               */
/* ============================================================ */
template <typename K, typename V, size_t H>
int32_t SkipList<K, V, H>::Put(const K &key, const V &value)
{
    GOECompareRes res = GOECompareRes::GREATER;
    list_head *searchRes[H];
    findGENode(key, searchRes, res);
    Printf("Put res %d\n", res);

    if (searchRes[0] != &head[0] && res == GOECompareRes::EQUAL)
    {
        auto *same = template_list_entry(searchRes[0], &ListNode<K, V, H>::list);
        same->UpdateValue(value);
    }
    else
    {
        ListNode<K, V, H> *node = createNewListNode<K, V, H>(key, value);
        if (node == NULL)
        {
            return SKIPLIST_NO_MEM;
        }

        std::uniform_int_distribution<int> dist(1, mHeight);
        auto desHeight = dist(mRng);
        Printf("desHeight %u\n", desHeight);
        for (int i = 0; i < mHeight; ++i)
        {
            if (i < desHeight)
            {
                Printf("insert height %u\n", i);
                list_add_prev(&node->list[i], searchRes[i]);
            }
            else
            {
                list_init(&node->list[i]);
            }
        }
    }
    Printf("GOECompareRes %d\n", res);
    return SKIPLIST_OK;
}

/* ============================================================ */
/* 查询接口                                                      */
/* ============================================================ */
template <typename K, typename V, size_t H>
const V* SkipList<K, V, H>::GetValue(const K &key)
{
    GOECompareRes res;
    auto *node = findNode(key, res);
    return node ? &node->value : NULL;
}

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
/* Delete：删除指定 key 的节点                                    */
/* ============================================================ */
template <typename K, typename V, size_t H>
int32_t SkipList<K, V, H>::Delete(const K &key)
{
    GOECompareRes res = GOECompareRes::GREATER;
    list_head *searchRes[H];
    findGENode(key, searchRes, res);

    // 目标节点必须存在：第0层 searchRes[0] 不是头节点，且 key 相等
    if (searchRes[0] == &head[0] || res != GOECompareRes::EQUAL)
    {
        return SKIPLIST_ERR;    // 键不存在
    }

    // 通过第0层的 list 指针反解出节点地址
    auto *target = template_list_entry(searchRes[0], &ListNode<K, V, H>::list);

    // 从每一层链表中摘除该节点
    for (int i = 0; i < mHeight; ++i)
    {
        // 如果该层中 target 的 list[i] 已经初始化（即确实插入了该层）
        // 通过检查 next/prev 是否指向自身来判断是否为空（未使用）
        if (target->list[i].next != &target->list[i])
        {
            list_del(&target->list[i]);
        }
    }

    delete target;
    return SKIPLIST_OK;
}

/* ============================================================ */
/* 显式实例化                                                    */
/* ============================================================ */
template class SkipList<int, int, 8>;
template class SkipList<std::vector<int>, int, 8>;
template class SkipList<std::string, int, 8>;
