#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>
#include <atomic>
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
/**
 * @brief 跳表插入和顺序测试用例
 * 测试跳表的插入功能以及键值按顺序排列的正确性
 */
TEST(SkipListIntTest, InsertAndOrder)
{
    // 创建一个最大高度为8的int到int的跳表
    SkipList<int, int, 8> sl;
    // 定义一组测试键值
    std::vector<int> keys = {42, 17, 89, 3, 56, 21, 78, 5, 99, 34};
    // 遍历键值，将每个键及其对应的值(键*10)插入跳表
    for (int k : keys)
    {
        sl.Put(k, k * 10);
    }

    // 验证第0层的键是否严格升序排列
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<0>()));
    // 验证第1层的键是否严格升序排列
    EXPECT_TRUE(IsStrictlyAscending(sl.GetKeysAtHeight<1>()));

    // 对原始键值进行排序
    std::sort(keys.begin(), keys.end());
    // 验证跳表第0层的键是否与排序后的键值一致
    EXPECT_EQ(sl.GetKeysAtHeight<0>(), keys);
}

TEST(SkipListIntTest, DuplicateUpdate)
{
    SkipList<int, int, 8> sl;
    sl.Put(100, 1);
    sl.Put(200, 2);
    sl.Put(100, 999);

    {
        int val = 0;
        EXPECT_EQ(sl.Get(100, val), SKIPLIST_OK);
        EXPECT_EQ(val, 999);
    }

    sl.Put(100, 12345);
    {
        int val = 0;
        EXPECT_EQ(sl.Get(100, val), SKIPLIST_OK);
        EXPECT_EQ(val, 12345);
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
        int val = 0;
        EXPECT_EQ(sl.Get(std::vector<int>{1, 2, 3}, val), SKIPLIST_OK);
        EXPECT_EQ(val, 15);
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
        int val = 0;
        EXPECT_EQ(sl.Get("apple", val), SKIPLIST_OK);
        EXPECT_EQ(val, 6);
    }
}

/* ============================================================ */
/* 一写多读并发测试                                              */
/* ============================================================ */
TEST(SkipListConcurrencyTest, SingleWriterMultiReader)
{
    SkipList<int, int, 8> sl;

    // 预填充数据
    for (int i = 1; i <= 100; ++i)
    {
        sl.Put(i, i * 10);
    }

    // 并发读者 + 单写者
    std::vector<std::thread> readers;
    std::atomic<bool> stop{false};

    // 启动 4 个读线程：只做遍历，不验证精确值（值在并发变化）
    for (int t = 0; t < 4; ++t)
    {
        readers.emplace_back([&sl, &stop]()
        {
            while (!stop.load())
            {
                for (int i = 1; i <= 100; ++i)
                {
                    int val = 0;
                    // Get 可能返回 OK 或 ERR（如果 key 被删除），但不应该崩溃
                    (void)sl.Get(i, val);
                }

                // 范围查询：只验证不崩溃，不验证精确值
                auto result = sl.RangeQuery(25, 75);
                for (size_t i = 1; i < result.size(); ++i)
                {
                    // 验证顺序性：结果必须升序
                    EXPECT_LT(result[i - 1].first, result[i].first);
                }
            }
        });
    }

    // 写线程：更新部分 key
    for (int round = 0; round < 50; ++round)
    {
        for (int i = 1; i <= 50; ++i)
        {
            sl.Put(i, i * 100 + round);
        }
    }

    // 停止读者
    stop.store(true);
    for (auto& t : readers)
    {
        t.join();
    }
}

TEST(SkipListConcurrencyTest, WriterDeleteWhileReaders)
{
    SkipList<int, int, 8> sl;

    for (int i = 1; i <= 50; ++i)
    {
        sl.Put(i, i * 10);
    }

    std::atomic<bool> stop{false};
    std::vector<std::thread> readers;

    for (int t = 0; t < 4; ++t)
    {
        readers.emplace_back([&sl, &stop]()
        {
            while (!stop.load())
            {
                for (int i = 1; i <= 50; ++i)
                {
                    int val = 0;
                    sl.Get(i, val);  // 可能返回 SKIPLIST_ERR（已删除）
                }
            }
        });
    }

    // 写线程删除一半 key
    for (int i = 1; i <= 25; ++i)
    {
        EXPECT_EQ(sl.Delete(i), SKIPLIST_OK);
    }

    // 验证删除的 key 不再被读到
    for (int i = 1; i <= 25; ++i)
    {
        int val = 0;
        EXPECT_EQ(sl.Get(i, val), SKIPLIST_ERR);
    }

    // 验证未删除的 key 仍然可读
    for (int i = 26; i <= 50; ++i)
    {
        int val = 0;
        EXPECT_EQ(sl.Get(i, val), SKIPLIST_OK);
        EXPECT_EQ(val, i * 10);
    }

    stop.store(true);
    for (auto& t : readers)
    {
        t.join();
    }

    // Compact 后释放 tombstone 内存
    sl.Compact();
}
