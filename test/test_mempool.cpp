#include <gtest/gtest.h>
#include <vector>
#include <string>
#include "skiplist.h"
#include "mempool/slab_pool.h"
#include "mempool/mempool_manager.h"
#include "mempool/mock_dpdk_mempool.h"

/* ============================================================ */
/* SlabPool 独立测试                                             */
/* ============================================================ */
TEST(SlabPoolTest, BasicAllocFree)
{
    SlabPool pool(64, 10, 8);
    EXPECT_EQ(pool.Capacity(), 10);
    EXPECT_EQ(pool.Used(), 0);

    void *p1 = pool.Alloc();
    EXPECT_NE(p1, nullptr);
    EXPECT_EQ(pool.Used(), 1);

    void *p2 = pool.Alloc();
    EXPECT_NE(p2, nullptr);
    EXPECT_EQ(pool.Used(), 2);

    pool.Free(p1);
    EXPECT_EQ(pool.Used(), 1);

    pool.Free(p2);
    EXPECT_EQ(pool.Used(), 0);
}

TEST(SlabPoolTest, ExhaustAndReuse)
{
    SlabPool pool(64, 3, 8);
    void *p1 = pool.Alloc();
    void *p2 = pool.Alloc();
    void *p3 = pool.Alloc();
    EXPECT_EQ(pool.Used(), 3);
    (void)p1; (void)p3;

    void *p4 = pool.Alloc();
    EXPECT_EQ(p4, nullptr);

    pool.Free(p2);
    void *p5 = pool.Alloc();
    EXPECT_NE(p5, nullptr);
    EXPECT_EQ(pool.Used(), 3);
}

/* ============================================================ */
/* MempoolManager + SkipList 集成测试                            */
/* ============================================================ */
TEST(SkipListWithMempoolManager, PutGetDelete)
{
    MemTypeConfig configs[] = {
        { MemType::SKIPLIST_NODE, sizeof(ListNode<int, int, 8>), 100, 8 },
    };
    MempoolManager mgr(configs, 1);
    SkipList<int, int, 8> sl;
    sl.SetMempool(&mgr);

    for (int i = 1; i <= 20; ++i)
    {
        sl.Put(i, i * 10);
    }

    for (int i = 1; i <= 20; ++i)
    {
        int val = 0;
        EXPECT_EQ(sl.Get(i, val), SKIPLIST_OK);
        EXPECT_EQ(val, i * 10);
    }

    for (int i = 1; i <= 20; i += 2)
    {
        EXPECT_EQ(sl.Delete(i), SKIPLIST_OK);
    }

    for (int i = 1; i <= 20; i += 2)
    {
        EXPECT_FALSE(sl.Find(i));
    }

    for (int i = 2; i <= 20; i += 2)
    {
        int val = 0;
        EXPECT_EQ(sl.Get(i, val), SKIPLIST_OK);
        EXPECT_EQ(val, i * 10);
    }
}

TEST(SkipListWithMempoolManager, ExhaustPool)
{
    MemTypeConfig configs[] = {
        { MemType::SKIPLIST_NODE, sizeof(ListNode<int, int, 8>), 5, 8 },
    };
    MempoolManager mgr(configs, 1);
    SkipList<int, int, 8> sl;
    sl.SetMempool(&mgr);

    for (int i = 1; i <= 5; ++i)
    {
        EXPECT_EQ(sl.Put(i, i), SKIPLIST_OK);
    }

    EXPECT_EQ(sl.Put(6, 6), SKIPLIST_NO_MEM);
}

/* ============================================================ */
/* MockDPDK + SkipList 集成测试                                  */
/* ============================================================ */
TEST(SkipListWithMockDpdk, PutGetDelete)
{
    MockDpdkMempool pool;
    SkipList<int, int, 8> sl;
    sl.SetMempool(&pool);

    for (int i = 1; i <= 10; ++i)
    {
        sl.Put(i, i * 10);
    }

    for (int i = 1; i <= 10; ++i)
    {
        int val = 0;
        EXPECT_EQ(sl.Get(i, val), SKIPLIST_OK);
        EXPECT_EQ(val, i * 10);
    }

    for (int i = 1; i <= 10; i += 2)
    {
        EXPECT_EQ(sl.Delete(i), SKIPLIST_OK);
    }

    for (int i = 2; i <= 10; i += 2)
    {
        int val = 0;
        EXPECT_EQ(sl.Get(i, val), SKIPLIST_OK);
        EXPECT_EQ(val, i * 10);
    }

    EXPECT_EQ(pool.AllocCount(), pool.FreeCount() + 5);
}

/* ============================================================ */
/* 无内存池回退测试                                              */
/* ============================================================ */
TEST(SkipListDefaultPool, NoPoolFallback)
{
    SkipList<int, int, 8> sl;
    for (int i = 1; i <= 100; ++i)
    {
        sl.Put(i, i * 10);
    }

    for (int i = 1; i <= 100; ++i)
    {
        int val = 0;
        EXPECT_EQ(sl.Get(i, val), SKIPLIST_OK);
        EXPECT_EQ(val, i * 10);
    }
}
