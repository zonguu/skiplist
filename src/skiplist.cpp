#include <cstdint>
#include <cstdlib>
#include <stdio.h>

#include "skiplist.h"
#include "list_template.h"
#include "likely/likely.h"

bool debugSwitch = false;

template class SkipList<int, int, 8>;

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
    auto *node = static_cast<ListNode<K, V, N> *>(SKIPLIST_MALLOC(sizeof(ListNode<K, V, N>)));
    if (node == NULL)
    {
        return NULL;
    }

    node->Init(key, value);
    return node;
}

template <typename K, typename V, size_t H>
int32_t SkipList<K, V, H>::Put(const K &key, const V &value)
{
    GOECompareRes res;
    std::vector<list_head*> searchRes(mHeight, NULL);
    findGENode(key, searchRes, res);
Printf("Put res %d\n", res);
    if (searchRes[0] != &head[0] && res == GOECompareRes::EQUAL)
    {
        auto *same = template_list_entry(searchRes[0], &ListNode<K, V, H>::list);
        std::cout << "redup \t" << key << "\t" << value << "\n";
        same->UpdateValue(value);
    }
    else
    {
        ListNode<K, V, H> *node = createNewListNode<K, V, H>(key, value);
        if (node == NULL)
        {
            return SKIPLIST_NO_MEM;
        }

        uint32_t desHeight = mRandomer.Rand(mHeight - 1) + 1;
        Printf("desHeight %u\n", desHeight);
        for (uint32_t i = 0; i < mHeight; ++i)
        {
            if (i < desHeight)
            {   Printf("insert height %u\n", i);
                list_add_prev(&node->list[i], searchRes[i]);
            }
            else
            {
                list_init(&node->list[i]);
            }
        }
    }
    std::cout << "GOECompareRes " << res << "\n";
    return SKIPLIST_OK;
}

template <typename K, typename V, size_t N>
void SkipList<K, V, N>::findGENode(const K & key, std::vector<list_head*> &searchRes, GOECompareRes &res)
{
    list_head *pos = NULL;
    const size_t offset = template_offsetof(&ListNode<K, V, N>::list);
    int height = searchRes.size() - 1;
    for (; height >= 0; --height)
    {
        Printf("find height %d\n", height);
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