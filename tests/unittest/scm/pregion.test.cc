#include "scm/pregion/pregion.h"
#include "common/errno.h"
#include <gtest/gtest.h>

#include <stdio.h>

// Suite: SCM
    TEST(SCM, TestPersistentRegion)
    {
        PersistentRegion* pregion1;
        PersistentRegion* pregion2;
        EXPECT_TRUE(PersistentRegion::Create("/tmp/persistent_region1", 1024 * 1024) == E_SUCCESS);
        EXPECT_TRUE(PersistentRegion::Create("/tmp/persistent_region2", 1024 * 1024) == E_SUCCESS);
        EXPECT_TRUE(PersistentRegion::Open("/tmp/persistent_region1", &pregion1) == E_SUCCESS);
        EXPECT_TRUE(pregion1->base() == 0x100000000000LLU);
        EXPECT_TRUE(PersistentRegion::Open("/tmp/persistent_region2", &pregion2) == E_SUCCESS);
        EXPECT_TRUE(pregion2->base() == 0x100000100000LLU);
        EXPECT_TRUE(PersistentRegion::Close(pregion1) == E_SUCCESS);
        EXPECT_TRUE(PersistentRegion::Close(pregion2) == E_SUCCESS);

        // Remap: region1, then region2
        EXPECT_TRUE(PersistentRegion::Open("/tmp/persistent_region1", &pregion1) == E_SUCCESS);
        EXPECT_TRUE(pregion1->base() == 0x100000000000LLU);
        EXPECT_TRUE(PersistentRegion::Open("/tmp/persistent_region2", &pregion2) == E_SUCCESS);
        EXPECT_TRUE(pregion2->base() == 0x100000100000LLU);
        EXPECT_TRUE(PersistentRegion::Close(pregion1) == E_SUCCESS);
        EXPECT_TRUE(PersistentRegion::Close(pregion2) == E_SUCCESS);

        // Remap: region2, then region1
        EXPECT_TRUE(PersistentRegion::Open("/tmp/persistent_region2", &pregion2) == E_SUCCESS);
        EXPECT_TRUE(pregion2->base() == 0x100000100000LLU);
        EXPECT_TRUE(PersistentRegion::Open("/tmp/persistent_region1", &pregion1) == E_SUCCESS);
        EXPECT_TRUE(pregion1->base() == 0x100000000000LLU);
        EXPECT_TRUE(PersistentRegion::Close(pregion1) == E_SUCCESS);
        EXPECT_TRUE(PersistentRegion::Close(pregion2) == E_SUCCESS);
    }
