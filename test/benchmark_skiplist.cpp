#include <gtest/gtest.h>
#include <map>
#include <random>
#include <string>
#include <vector>
#include "skiplist.h"

/* ============================================================ */
/* int 键：大规模随机 Put/Get 一致性 benchmark                   */
/* ============================================================ */
TEST(BenchmarkInt, PutGetConsistency)
{
    SkipList<int, int, 8> sl;
    std::map<int, int> expected;

    std::mt19937 rng(42);                       // 固定种子，可复现
    std::uniform_int_distribution<int> keyDist(0, 99999);
    std::uniform_int_distribution<int> valDist(1, 1000000); // 避开 0，方便区分默认值

    const int N = 5000;

    // Phase 1: 随机 Put
    for (int i = 0; i < N; ++i)
    {
        int key = keyDist(rng);
        int val = valDist(rng);
        sl.Put(key, val);
        expected[key] = val;
    }

    // Phase 2: 验证所有已插入键的 Get 结果
    for (const auto& [key, val] : expected)
    {
        int outVal = 0;
        EXPECT_EQ(sl.Get(key, outVal), SKIPLIST_OK) << "Get failed at key " << key;
        EXPECT_EQ(outVal, val) << "Mismatch at key " << key;
    }

    // Phase 3: 随机 Get（含不存在的键）
    for (int i = 0; i < N; ++i)
    {
        int key = keyDist(rng);
        auto it = expected.find(key);
        int outVal = 0;
        if (it != expected.end())
        {
            EXPECT_EQ(sl.Get(key, outVal), SKIPLIST_OK) << "Get failed at key " << key;
            EXPECT_EQ(outVal, it->second) << "Mismatch at key " << key;
        }
        else
        {
            EXPECT_EQ(sl.Get(key, outVal), SKIPLIST_ERR) << "Key " << key << " should not exist";
        }
    }

    // Phase 4: Put/Get 混合压力测试
    std::uniform_int_distribution<int> opDist(0, 1); // 0: put, 1: get
    for (int i = 0; i < N; ++i)
    {
        int key = keyDist(rng);
        if (opDist(rng) == 0)
        {
            int val = valDist(rng);
            sl.Put(key, val);
            expected[key] = val;
        }
        else
        {
            auto it = expected.find(key);
            int outVal = 0;
            if (it != expected.end())
            {
                EXPECT_EQ(sl.Get(key, outVal), SKIPLIST_OK) << "Get failed at key " << key;
                EXPECT_EQ(outVal, it->second) << "Mismatch at key " << key;
            }
            else
            {
                EXPECT_EQ(sl.Get(key, outVal), SKIPLIST_ERR) << "Key " << key << " should not exist";
            }
        }
    }

    // Phase 5: 最终全量校验
    for (const auto& [key, val] : expected)
    {
        int outVal = 0;
        EXPECT_EQ(sl.Get(key, outVal), SKIPLIST_OK) << "Final Get failed at key " << key;
        EXPECT_EQ(outVal, val) << "Final mismatch at key " << key;
    }
}

/* ============================================================ */
/* std::vector<int> 多维键：随机 Put/Get 一致性                   */
/* ============================================================ */
TEST(BenchmarkVector, PutGetConsistency)
{
    SkipList<std::vector<int>, int, 8> sl;
    std::map<std::vector<int>, int> expected;

    std::mt19937 rng(123);
    std::uniform_int_distribution<int> dimDist(1, 1000);

    const int N = 2000;

    auto makeKey = [&]() -> std::vector<int>
    {
        return { dimDist(rng), dimDist(rng), dimDist(rng) };
    };

    for (int i = 0; i < N; ++i)
    {
        auto key = makeKey();
        int val = dimDist(rng);
        sl.Put(key, val);
        expected[key] = val;
    }

    for (const auto& [key, val] : expected)
    {
        int outVal = 0;
        EXPECT_EQ(sl.Get(key, outVal), SKIPLIST_OK);
        EXPECT_EQ(outVal, val) << "Vector key mismatch";
    }

    // 混合测试
    for (int i = 0; i < N; ++i)
    {
        auto key = makeKey();
        int val = dimDist(rng);
        sl.Put(key, val);
        expected[key] = val;

        if (i % 2 == 0)
        {
            auto checkKey = makeKey();
            auto it = expected.find(checkKey);
            int outVal = 0;
            if (it != expected.end())
            {
                EXPECT_EQ(sl.Get(checkKey, outVal), SKIPLIST_OK);
                EXPECT_EQ(outVal, it->second);
            }
        }
    }

    for (const auto& [key, val] : expected)
    {
        int outVal = 0;
        EXPECT_EQ(sl.Get(key, outVal), SKIPLIST_OK);
        EXPECT_EQ(outVal, val) << "Final vector key mismatch";
    }
}

/* ============================================================ */
/* std::string 键：随机 Put/Get 一致性                            */
/* ============================================================ */
TEST(BenchmarkString, PutGetConsistency)
{
    SkipList<std::string, int, 8> sl;
    std::map<std::string, int> expected;

    std::mt19937 rng(999);
    const char* words[] = {
        "apple", "banana", "cherry", "date", "elderberry",
        "fig", "grape", "honeydew", "kiwi", "lemon",
        "mango", "nectarine", "orange", "papaya", "quince"
    };
    const int wordCount = sizeof(words) / sizeof(words[0]);
    std::uniform_int_distribution<int> wordDist(0, wordCount - 1);
    std::uniform_int_distribution<int> valDist(1, 100000);

    const int N = 3000;

    auto makeKey = [&]() -> std::string
    {
        return std::string(words[wordDist(rng)]) + "_" + std::to_string(valDist(rng));
    };

    for (int i = 0; i < N; ++i)
    {
        auto key = makeKey();
        int val = valDist(rng);
        sl.Put(key, val);
        expected[key] = val;
    }

    for (const auto& [key, val] : expected)
    {
        int outVal = 0;
        EXPECT_EQ(sl.Get(key, outVal), SKIPLIST_OK);
        EXPECT_EQ(outVal, val) << "String key mismatch: " << key;
    }

    // 混合测试
    for (int i = 0; i < N; ++i)
    {
        auto key = makeKey();
        int val = valDist(rng);
        sl.Put(key, val);
        expected[key] = val;

        if (i % 3 == 0)
        {
            auto checkKey = makeKey();
            auto it = expected.find(checkKey);
            int outVal = 0;
            if (it != expected.end())
            {
                EXPECT_EQ(sl.Get(checkKey, outVal), SKIPLIST_OK);
                EXPECT_EQ(outVal, it->second) << "String key mismatch: " << checkKey;
            }
        }
    }

    for (const auto& [key, val] : expected)
    {
        int outVal = 0;
        EXPECT_EQ(sl.Get(key, outVal), SKIPLIST_OK);
        EXPECT_EQ(outVal, val) << "Final string key mismatch: " << key;
    }
}
