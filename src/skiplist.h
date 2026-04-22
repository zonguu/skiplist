#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <vector>
#include <iostream>
#include <string>
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
bool GOE(K *a, const K & key);

template <typename K>
bool GOE(const K *a, const K & key, GOECompareRes &res);

template <typename K>
bool GOE(const K &a, const K &b, GOECompareRes &res);

template <typename K, typename V, size_t N>
ListNode<K, V, N>* createNewListNode(const K &key, const V &value);

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
    SkipList();
    ~SkipList();

    int32_t Put(const K &key, const V &value);

    int32_t skiplistInsert(const K & key, const V &value, list_head* path);
    void findGENode(const K & key, std::vector<list_head*> &searchRes, GOECompareRes &res);

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

    V GetValue(const K &key);

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
};

#endif /* SKIPLIST_H */
