#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm>
#include "skiplist.h"

/* ============================================================ */
/* RangeQuery 基础测试                                           */
/* ============================================================ */
TEST(SkipListIntTest, RangeQueryBasic)
{
    SkipList<int, int, 8> sl;
    for (int i = 1; i <= 10; ++i)
    {
        sl.Put(i * 10, i * 100);
    }

    // [30, 70] → 30,40,50,60,70
    auto result = sl.RangeQuery(30, 70);
    EXPECT_EQ(result.size(), 5);
    EXPECT_EQ(result[0].first, 30);
    EXPECT_EQ(result[0].second, 300);
    EXPECT_EQ(result[4].first, 70);
    EXPECT_EQ(result[4].second, 700);
}

TEST(SkipListIntTest, RangeQueryExactBoundary)
{
    SkipList<int, int, 8> sl;
    sl.Put(10, 100);
    sl.Put(20, 200);
    sl.Put(30, 300);

    // [10, 10] → 只包含 10
    auto result = sl.RangeQuery(10, 10);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].first, 10);

    // [20, 20] → 只包含 20
    result = sl.RangeQuery(20, 20);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].first, 20);
}

TEST(SkipListIntTest, RangeQueryEmpty)
{
    SkipList<int, int, 8> sl;
    sl.Put(10, 100);
    sl.Put(30, 300);
    sl.Put(50, 500);

    // [15, 25] → 空
    auto result = sl.RangeQuery(15, 25);
    EXPECT_EQ(result.size(), 0);

    // [60, 100] → 空
    result = sl.RangeQuery(60, 100);
    EXPECT_EQ(result.size(), 0);
}

TEST(SkipListIntTest, RangeQueryStartNotExist)
{
    SkipList<int, int, 8> sl;
    sl.Put(10, 100);
    sl.Put(30, 300);
    sl.Put(50, 500);
    sl.Put(70, 700);

    // start=25 不存在，从 30 开始
    auto result = sl.RangeQuery(25, 60);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].first, 30);
    EXPECT_EQ(result[1].first, 50);
}

TEST(SkipListIntTest, RangeQueryAfterDelete)
{
    SkipList<int, int, 8> sl;
    for (int i = 1; i <= 10; ++i)
    {
        sl.Put(i * 10, i * 100);
    }

    sl.Delete(50);
    sl.Delete(60);

    auto result = sl.RangeQuery(30, 80);
    EXPECT_EQ(result.size(), 4);
    EXPECT_EQ(result[0].first, 30);
    EXPECT_EQ(result[1].first, 40);
    EXPECT_EQ(result[2].first, 70);
    EXPECT_EQ(result[3].first, 80);
}

/* ============================================================ */
/* RangeQuery 全量扫描                                           */
/* ============================================================ */
TEST(SkipListIntTest, RangeQueryFullScan)
{
    SkipList<int, int, 8> sl;
    for (int i = 1; i <= 100; ++i)
    {
        sl.Put(i, i * 10);
    }

    auto result = sl.RangeQuery(1, 100);
    EXPECT_EQ(result.size(), 100);
    for (size_t i = 0; i < result.size(); ++i)
    {
        EXPECT_EQ(result[i].first, static_cast<int>(i + 1));
        EXPECT_EQ(result[i].second, static_cast<int>((i + 1) * 10));
    }
}

/* ============================================================ */
/* RangeQuery 与 std::string 键                                  */
/* ============================================================ */
TEST(SkipListStringTest, RangeQueryStringKey)
{
    SkipList<std::string, int, 8> sl;
    sl.Put("apple", 1);
    sl.Put("banana", 2);
    sl.Put("cherry", 3);
    sl.Put("date", 4);
    sl.Put("elderberry", 5);

    auto result = sl.RangeQuery("banana", "date");
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].first, "banana");
    EXPECT_EQ(result[1].first, "cherry");
    EXPECT_EQ(result[2].first, "date");
}

/* ============================================================ */
/* RangeQuery 与 std::vector<int> 键                             */
/* ============================================================ */
TEST(SkipListVectorTest, RangeQueryVectorKey)
{
    SkipList<std::vector<int>, int, 8> sl;
    sl.Put({1, 1}, 11);
    sl.Put({1, 2}, 12);
    sl.Put({1, 3}, 13);
    sl.Put({2, 0}, 20);
    sl.Put({2, 1}, 21);

    auto result = sl.RangeQuery(std::vector<int>{1, 2}, std::vector<int>{2, 0});
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].first, (std::vector<int>{1, 2}));
    EXPECT_EQ(result[1].first, (std::vector<int>{1, 3}));
    EXPECT_EQ(result[2].first, (std::vector<int>{2, 0}));
}
