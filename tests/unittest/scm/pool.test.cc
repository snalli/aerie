#include "scm/pool/pool.h"
#include "common/errno.h"
#include <gtest/gtest.h>

#include <stdio.h>

// Suite: SCM
    TEST(SCM, TestPool)
    {
        StoragePool* pool1;
        void* ptr;

        EXPECT_TRUE(StoragePool::Create("/tmp/stamnos_pool", 1024 * 1024 * 1024, 0) == E_SUCCESS);
        EXPECT_TRUE(StoragePool::Open("/tmp/stamnos_pool", &pool1) == E_SUCCESS);

        EXPECT_TRUE(pool1->AllocateExtent(4096 * 2, &ptr) == E_SUCCESS);
        EXPECT_TRUE(pool1->AllocateExtent(4096 * 5, &ptr) == E_SUCCESS);

        EXPECT_TRUE(StoragePool::Close(pool1) == E_SUCCESS);

        EXPECT_TRUE(StoragePool::Open("/tmp/stamnos_pool", &pool1) == E_SUCCESS);
        EXPECT_TRUE(pool1->AllocateExtent(4096 * 3, &ptr) == E_SUCCESS);
    }
