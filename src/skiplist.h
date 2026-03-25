#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <vector>
#include <iostream>
#include <assert.h>

#include "stdint.h"
#include "list.h"
#include "list_template.h"
#include "randomer/randomer.h"

extern bool debugSwitch;

#define Printf(...) \
    do { if (debugSwitch) \
    printf(__VA_ARGS__); \
} while (0)

#define SKIPLIST_MAX_HEIGHT (8)

#define SKIPLIST_MALLOC(size)   malloc(size)
#define SKIPLIST_FREE(ptr)      free(ptr)

#define SKIPLIST_OK (0)
#define SKIPLIST_ERR (-1)
#define SKIPLIST_NO_MEM (-2)

template <typename K, typename V, size_t N>
struct ListNode
{
    K key;
    V value;
    struct list_head list[N];
    ListNode(K &k, V &v) : key(k), value(v)
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
    SKIPLIST_FREE(node);
}

enum GOECompareRes
{
    LESS = 0,
    EQUAL = 1,
    GREATER = 2,
};


template <typename K, typename V, size_t H>
class SkipList
{
    static_assert(H >= 1, "SkipList height H must be >= 1");

private:
    int32_t     mHeight;
    Randomer    mRandomer;
    list_head   head[H];

private:

public:
    SkipList() : mHeight(H)
    {
        for (size_t i = 0; i < H; ++i)
        {
            list_init(&head[i]);
        }
    }

    ~SkipList()
    {
        template_list_for_each_entry_array_safe<ListNode<K, V, H>, list_head, 0, H>(&head[0], &ListNode<K, V, H>::list, FreeListNode<K, V, H>);
    }

    int32_t Put(const K &key, const V &value);

    int32_t skiplistInsert(const K & key, const V &value, list_head* path);
    void findGENode(const K & key, std::vector<list_head*> &searchRes, GOECompareRes &res);

    template<size_t h>
    void DisplaySpecifiedHeight()
    {
        std::cout << "display\n";
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

    // void DisplayReverse()
    // {
    //     std::cout << "display reverse \n";
    //     template_list_for_each_entry_reverse<ListNode<K, V, H>, list_head>(&head[0], &ListNode<K, V, H>::list, [](ListNode<K, V, H> *node)
    //                                                                     { std::cout << node->key << "\t" << node->value << std::endl; });
    // }

    // int32_t Get(const K &key, V &value);
    // int32_t insert(const K &key, const V &value);
};

#endif /* SKIPLIST_H */
