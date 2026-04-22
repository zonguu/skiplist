#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm>
#include "skiplist.h"

/* ============================================================ */
/* 辅助函数：检查一个序列是否严格升序                            */
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
/* int 类型键测试                                                */
/* ============================================================ */
TEST(SkipListIntTest, InsertAndOrder)
{
    SkipList<int, int, 8> sl;
    std::vector<int> keys = {42, 17, 89, 3, 56, 21, 78, 5, 99, 34};
    for (int k : keys)
    {
        sl.Put(k, k * 10);
    }

    // 每一层的键都必须保持升序
    auto keys0 = sl.GetKeysAtHeight<0>();
    auto keys1 = sl.GetKeysAtHeight<1>();
    auto keys2 = sl.GetKeysAtHeight<2>();
    auto keys3 = sl.GetKeysAtHeight<3>();
    auto keys4 = sl.GetKeysAtHeight<4>();
    auto keys5 = sl.GetKeysAtHeight<5>();
    auto keys6 = sl.GetKeysAtHeight<6>();
    auto keys7 = sl.GetKeysAtHeight<7>();

    EXPECT_TRUE(IsStrictlyAscending(keys0));
    EXPECT_TRUE(IsStrictlyAscending(keys1));
    EXPECT_TRUE(IsStrictlyAscending(keys2));
    EXPECT_TRUE(IsStrictlyAscending(keys3));
    EXPECT_TRUE(IsStrictlyAscending(keys4));
    EXPECT_TRUE(IsStrictlyAscending(keys5));
    EXPECT_TRUE(IsStrictlyAscending(keys6));
    EXPECT_TRUE(IsStrictlyAscending(keys7));

    // 高层节点必须是低层节点的子集
    auto isSubset = [](const std::vector<int>& small, const std::vector<int>& big)
    {
        size_t i = 0, j = 0;
        while (i < small.size() && j < big.size())
        {
            if (small[i] == big[j]) { ++i; ++j; }
            else if (big[j] < small[i]) { ++j; }
            else { return false; }
        }
        return i == small.size();
    };

    EXPECT_TRUE(isSubset(keys1, keys0));
    EXPECT_TRUE(isSubset(keys2, keys1));
    EXPECT_TRUE(isSubset(keys3, keys2));
    EXPECT_TRUE(isSubset(keys4, keys3));
    EXPECT_TRUE(isSubset(keys5, keys4));
    EXPECT_TRUE(isSubset(keys6, keys5));
    EXPECT_TRUE(isSubset(keys7, keys6));

    // 第0层必须包含所有插入的键
    std::sort(keys.begin(), keys.end());
    EXPECT_EQ(keys0, keys);
}

TEST(SkipListIntTest, DuplicateUpdate)
{
    SkipList<int, int, 8> sl;
    sl.Put(100, 1);
    sl.Put(200, 2);
    sl.Put(100, 999);   // 重复键，应更新 value

    {
        auto *p = sl.GetValue(100);
        EXPECT_NE(p, nullptr);
        EXPECT_EQ(*p, 999);
    }
    {
        auto *p = sl.GetValue(200);
        EXPECT_NE(p, nullptr);
        EXPECT_EQ(*p, 2);
    }

    // 再次更新
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

    // Get 存在的键
    int val = 0;
    EXPECT_EQ(sl.Get(10, val), SKIPLIST_OK);
    EXPECT_EQ(val, 100);
    EXPECT_EQ(sl.Get(20, val), SKIPLIST_OK);
    EXPECT_EQ(val, 200);
    EXPECT_EQ(sl.Get(30, val), SKIPLIST_OK);
    EXPECT_EQ(val, 300);

    // Get 不存在的键
    EXPECT_EQ(sl.Get(99, val), SKIPLIST_ERR);

    // Find 存在的键
    EXPECT_TRUE(sl.Find(10));
    EXPECT_TRUE(sl.Find(20));
    EXPECT_TRUE(sl.Find(30));

    // Find 不存在的键
    EXPECT_FALSE(sl.Find(99));
    EXPECT_FALSE(sl.Find(0));
}

/* ============================================================ */
/* 多维 int 键测试（使用 std::vector<int>）                       */
/* ============================================================ */
TEST(SkipListVectorTest, MultiDimIntKey)
{
    SkipList<std::vector<int>, int, 8> sl;

    std::vector<std::vector<int>> keys = {
        {1, 2, 3},
        {1, 2, 4},
        {1, 3, 0},
        {2, 0, 0},
        {0, 9, 9},
        {1, 2, 3},  // duplicate
    };

    int val = 10;
    for (const auto& k : keys)
    {
        sl.Put(k, val++);
    }

    // 检查每一层升序
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<0>()));
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<1>()));
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<2>()));
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<3>()));

    // 检查重复键更新（最后一个 {1,2,3} 的 value 是 15）
    {
        auto *p = sl.GetValue(std::vector<int>{1, 2, 3});
        EXPECT_NE(p, nullptr);
        EXPECT_EQ(*p, 15);
    }
    {
        auto *p = sl.GetValue(std::vector<int>{2, 0, 0});
        EXPECT_NE(p, nullptr);
        EXPECT_EQ(*p, 13);
    }
}

/* ============================================================ */
/* string 类型键测试                                             */
/* ============================================================ */
TEST(SkipListStringTest, StringKey)
{
    SkipList<std::string, int, 8> sl;

    std::vector<std::string> keys = {
        "apple",
        "banana",
        "cherry",
        "date",
        "apricot",
        "apple",    // duplicate
    };

    int val = 1;
    for (const auto& k : keys)
    {
        sl.Put(k, val++);
    }

    // 检查每一层升序
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<0>()));
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<1>()));
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<2>()));
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<3>()));

    // 检查重复键更新（最后一个 "apple" 的 value 是 6）
    {
        auto *p = sl.GetValue("apple");
        EXPECT_NE(p, nullptr);
        EXPECT_EQ(*p, 6);
    }
    {
        auto *p = sl.GetValue("cherry");
        EXPECT_NE(p, nullptr);
        EXPECT_EQ(*p, 3);
    }
}