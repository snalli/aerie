#include "scm/pheap/pheap.h"
#include "common/errno.h"
#include <gtest/gtest.h>

#include <stdio.h>

// Suite: SCM
    TEST(SCM, TestPersistentHeap1)
    {
        PersistentHeap* pheap;
        int* ptr1;

        EXPECT_TRUE(PersistentHeap::Open("/tmp/persistent_heap1", 1024 * 1024, NULL,
                                   PersistentHeap::kReset, &pheap) == E_SUCCESS);

        EXPECT_TRUE(pheap->Alloc(512, (void**) &ptr1) == E_SUCCESS);
        *ptr1 = 0xc0ffee;
        EXPECT_TRUE(PersistentHeap::Close(pheap) == E_SUCCESS);

        // reincarnate the heap
        EXPECT_TRUE(PersistentHeap::Open("/tmp/persistent_heap1", 1024 * 1024, NULL, 0, &pheap) ==
              E_SUCCESS);

        EXPECT_TRUE(*ptr1 == 0xc0ffee);
        EXPECT_TRUE(PersistentHeap::Close(pheap) == E_SUCCESS);
    }

    TEST(SCM, TestPersistentHeap2)
    {
        PersistentHeap* pheap;
        int* ptr1;
        int* ptr2;

        EXPECT_TRUE(PersistentHeap::Open("/tmp/persistent_heap1", 1024 * 1024, NULL,
                                   PersistentHeap::kReset, &pheap) == E_SUCCESS);

        EXPECT_TRUE(pheap->Alloc(512, (void**) &ptr1) == E_SUCCESS);
        *ptr1 = 0xc0ffee;
        EXPECT_TRUE(PersistentHeap::Close(pheap) == E_SUCCESS);

        // reincarnate the heap
        EXPECT_TRUE(PersistentHeap::Open("/tmp/persistent_heap1", 1024 * 1024, NULL, 0, &pheap) ==
              E_SUCCESS);

        EXPECT_TRUE(*ptr1 == 0xc0ffee);

        EXPECT_TRUE(pheap->Alloc(512, (void**) &ptr2) == E_SUCCESS);
        *ptr2 = 0xbeef;
        EXPECT_TRUE(*ptr1 == 0xc0ffee);
        EXPECT_TRUE(*ptr2 == 0xbeef);
        EXPECT_TRUE(PersistentHeap::Close(pheap) == E_SUCCESS);
    }

    TEST(SCM, TestPersistentHeapRoot)
    {
        PersistentHeap* pheap;
        int* ptr1;
        int* ptr2;

        EXPECT_TRUE(PersistentHeap::Open("/tmp/persistent_heap1", 1024 * 1024, NULL,
                                   PersistentHeap::kReset, &pheap) == E_SUCCESS);

        EXPECT_TRUE(pheap->Alloc(512, (void**) &ptr1) == E_SUCCESS);
        *ptr1 = 0xc0ffee;
        pheap->set_root((void*) ptr1);
        EXPECT_TRUE(PersistentHeap::Close(pheap) == E_SUCCESS);

        // reincarnate the heap
        EXPECT_TRUE(PersistentHeap::Open("/tmp/persistent_heap1", 1024 * 1024, NULL, 0, &pheap) ==
              E_SUCCESS);
        EXPECT_TRUE(pheap->root() == (void*) ptr1);
        EXPECT_TRUE(PersistentHeap::Close(pheap) == E_SUCCESS);
    }
