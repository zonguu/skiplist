#include <cstdint>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <vector>

#include "skiplist.h"
#include "list_template.h"
#include "likely/likely.h"

bool debugSwitch = false;

template <typename K>
bool GOE(K *a, const K & key)
{
    return key < a->key;
}

template <typename K>
bool GOE(const K *a, const K & key, GOECompareRes &res)
{
    if (a->key < key)
    {
        res = GOECompareRes::LESS;
        return false;
    }
    else if (key < a->key)
    {
        res = GOECompareRes::GREATER;
    }
    else
    {
        res = GOECompareRes::EQUAL;
    }
    return true;
}

template <typename K>
bool GOE(const K &a, const K &b, GOECompareRes &res)
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

template <typename K, typename V, size_t N>
ListNode<K, V, N>* createNewListNode(const K &key, const V &value)
{
    return new ListNode<K, V, N>(key, value);
}

template <typename K, typename V, size_t H>
SkipList<K, V, H>::SkipList() : mHeight(H)
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

template <typename K, typename V, size_t H>
int32_t SkipList<K, V, H>::Put(const K &key, const V &value)
{
    GOECompareRes res = GOECompareRes::GREATER;
    std::vector<list_head*> searchRes(mHeight, NULL);
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

        auto desHeight = mRandomer.Rand(mHeight - 1) + 1;
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

template <typename K, typename V, size_t N>
void SkipList<K, V, N>::findGENode(const K & key, std::vector<list_head*> &searchRes, GOECompareRes &res)
{
    list_head *pos = NULL;
    int height = searchRes.size() - 1;
    for (; height >= 0; --height)
    {
        Printf("find height %d\n", height);
        const size_t offset = template_offsetof(&ListNode<K, V, N>::list) + height * sizeof(list_head);
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

template <typename K, typename V, size_t H>
V SkipList<K, V, H>::GetValue(const K &key)
{
    list_head *pos = NULL;
    const size_t offset = template_offsetof(&ListNode<K, V, H>::list);
    list_for_each(pos, &head[0])
    {
        auto *entry = reinterpret_cast<ListNode<K, V, H>*>(reinterpret_cast<char*>(pos) - offset);
        if (!(entry->key < key) && !(key < entry->key))
        {
            return entry->value;
        }
    }
    return V{};
}

/* ============================================================ */
/* 显式实例化：保证测试与 benchmark 用到的类型都能链接            */
/* ============================================================ */
template class SkipList<int, int, 8>;
template class SkipList<std::vector<int>, int, 8>;
template class SkipList<std::string, int, 8>;
