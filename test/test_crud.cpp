#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm>
#include "skiplist.h"

/* ============================================================ */
/* 辅助函数                                                      */
/* ============================================================ */
template<typename T>
bool IsStrictlyAscending(const std::vector<T>& vec)
{
    for (size_t i = 1; i < vec.size(); ++i)
    {
        if (!(vec[i - 1] < vec[i]))
        {
            return false;
        }
    }
    return true;
}

/* ============================================================ */
/* Put / Get / Find 基础测试                                     */
/* ============================================================ */
TEST(SkipListIntTest, InsertAndOrder)
{
    SkipList<int, int, 8> sl;
    std::vector<int> keys = {42, 17, 89, 3, 56, 21, 78, 5, 99, 34};
    for (int k : keys)
    {
        sl.Put(k, k * 10);
    }

    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<0>()));
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<1>()));

    std::sort(keys.begin(), keys.end());
    EXPECT_EQ(sl.GetKeysAtHeight<0>(), keys);
}

TEST(SkipListIntTest, DuplicateUpdate)
{
    SkipList<int, int, 8> sl;
    sl.Put(100, 1);
    sl.Put(200, 2);
    sl.Put(100, 999);

    {
        auto *p = sl.GetValue(100);
        EXPECT_NE(p, nullptr);
        EXPECT_EQ(*p, 999);
    }

    sl.Put(100, 12345);
    {
        auto *p = sl.GetValue(100);
        EXPECT_NE(p, nullptr);
        EXPECT_EQ(*p, 12345);
    }
}

TEST(SkipListIntTest, GetAndFind)
{
    SkipList<int, int, 8> sl;
    sl.Put(10, 100);
    sl.Put(20, 200);
    sl.Put(30, 300);

    int val = 0;
    EXPECT_EQ(sl.Get(10, val), SKIPLIST_OK);
    EXPECT_EQ(val, 100);
    EXPECT_EQ(sl.Get(99, val), SKIPLIST_ERR);

    EXPECT_TRUE(sl.Find(10));
    EXPECT_FALSE(sl.Find(99));
}

/* ============================================================ */
/* Delete 测试                                                   */
/* ============================================================ */
TEST(SkipListIntTest, DeleteBasic)
{
    SkipList<int, int, 8> sl;
    for (int i = 1; i <= 10; ++i)
    {
        sl.Put(i * 10, i * 100);
    }

    EXPECT_EQ(sl.Delete(50), SKIPLIST_OK);
    EXPECT_FALSE(sl.Find(50));

    EXPECT_EQ(sl.Delete(10), SKIPLIST_OK);
    EXPECT_EQ(sl.Delete(100), SKIPLIST_OK);

    EXPECT_EQ(sl.Delete(999), SKIPLIST_ERR);
    EXPECT_EQ(sl.Delete(50), SKIPLIST_ERR);

    for (int i = 2; i <= 9; ++i)
    {
        if (i == 5) continue;
        int v = 0;
        EXPECT_EQ(sl.Get(i * 10, v), SKIPLIST_OK);
        EXPECT_EQ(v, i * 100);
    }

    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<0>()));
}

TEST(SkipListIntTest, DeleteThenReinsert)
{
    SkipList<int, int, 8> sl;
    sl.Put(42, 420);
    sl.Put(17, 170);
    sl.Put(89, 890);

    EXPECT_EQ(sl.Delete(42), SKIPLIST_OK);
    EXPECT_FALSE(sl.Find(42));

    sl.Put(42, 999);
    int val = 0;
    EXPECT_EQ(sl.Get(42, val), SKIPLIST_OK);
    EXPECT_EQ(val, 999);

    EXPECT_EQ(sl.Delete(42), SKIPLIST_OK);
    EXPECT_FALSE(sl.Find(42));
}

TEST(SkipListIntTest, DeleteAllThenEmpty)
{
    SkipList<int, int, 8> sl;
    sl.Put(1, 10);
    sl.Put(2, 20);
    sl.Put(3, 30);

    EXPECT_EQ(sl.Delete(1), SKIPLIST_OK);
    EXPECT_EQ(sl.Delete(2), SKIPLIST_OK);
    EXPECT_EQ(sl.Delete(3), SKIPLIST_OK);

    EXPECT_FALSE(sl.Find(1));
    EXPECT_FALSE(sl.Find(2));
    EXPECT_FALSE(sl.Find(3));

    sl.Put(100, 1000);
    int val = 0;
    EXPECT_EQ(sl.Get(100, val), SKIPLIST_OK);
    EXPECT_EQ(val, 1000);
}

/* ============================================================ */
/* 多维 int 键测试                                               */
/* ============================================================ */
TEST(SkipListVectorTest, MultiDimIntKey)
{
    SkipList<std::vector<int>, int, 8> sl;

    std::vector<std::vector<int>> keys = {
        {1, 2, 3}, {1, 2, 4}, {1, 3, 0}, {2, 0, 0}, {0, 9, 9}, {1, 2, 3},
    };

    int val = 10;
    for (const auto& k : keys)
    {
        sl.Put(k, val++);
    }

    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<0>()));
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<1>()));

    {
        auto *p = sl.GetValue(std::vector<int>{1, 2, 3});
        EXPECT_NE(p, nullptr);
        EXPECT_EQ(*p, 15);
    }
}

/* ============================================================ */
/* string 类型键测试                                             */
/* ============================================================ */
TEST(SkipListStringTest, StringKey)
{
    SkipList<std::string, int, 8> sl;

    std::vector<std::string> keys = {
        "apple", "banana", "cherry", "date", "apricot", "apple",
    };

    int val = 1;
    for (const auto& k : keys)
    {
        sl.Put(k, val++);
    }

    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<0>()));
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<1>()));

    {
        auto *p = sl.GetValue("apple");
        EXPECT_NE(p, nullptr);
        EXPECT_EQ(*p, 6);
    }
}
