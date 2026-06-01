#include "pxfs/client/file.h"
#include "common/errno.h"
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>

using namespace client;

// Suite: File
    TEST(File, Alloc1)
    {
        File* fp = nullptr;
        FileManager* my_fmgr = new FileManager(1000, 2000);

        my_fmgr->Init();
        EXPECT_TRUE(my_fmgr->AllocFile(&fp) == 0);
        EXPECT_TRUE(my_fmgr->AllocFd(fp) == 1000);
        EXPECT_TRUE(my_fmgr->Put(1000) == 0);
        EXPECT_TRUE(my_fmgr->Put(1000) == -1);
    }

    TEST(File, Alloc2)
    {
        File* fp = nullptr;
        FileManager* my_fmgr = new FileManager(1000, 2000);

        my_fmgr->Init();
        EXPECT_TRUE(my_fmgr->AllocFile(&fp) == 0);
        EXPECT_TRUE(my_fmgr->AllocFd(fp) == 1000);
        EXPECT_TRUE(my_fmgr->Get(1000, &fp) == 1001);
        EXPECT_TRUE(my_fmgr->Put(1000) == 0);
        EXPECT_TRUE(my_fmgr->Put(1001) == 0);
    }

// Suite: FileDescriptor
    TEST(FileDescriptor, AllocSingle)
    {
        File* fp = nullptr;
        FileManager* my_fmgr = new FileManager(1000, 2000);

        my_fmgr->Init();
        EXPECT_TRUE(my_fmgr->AllocFd(fp) == 1000);
    }

    TEST(FileDescriptor, AllocMultiple1)
    {
        File* fp = nullptr;
        FileManager* my_fmgr = new FileManager(1000, 1004);

        my_fmgr->Init();
        EXPECT_TRUE(my_fmgr->AllocFd(fp) == 1000);
        EXPECT_TRUE(my_fmgr->AllocFd(fp) == 1001);
        EXPECT_TRUE(my_fmgr->AllocFd(fp) == 1002);
        EXPECT_TRUE(my_fmgr->AllocFd(fp) == 1003);
        EXPECT_TRUE(my_fmgr->AllocFd(fp) < 0);
        EXPECT_TRUE(my_fmgr->Put(999) < 0);
        EXPECT_TRUE(my_fmgr->Put(1004) < 0);
    }

    TEST(FileDescriptor, AllocMultiple2)
    {
        File* fp = nullptr;
        FileManager* my_fmgr = new FileManager(1000, 1004);

        my_fmgr->Init();
        my_fmgr->AllocFile(&fp);
        EXPECT_TRUE(my_fmgr->AllocFd(fp) == 1000);
        EXPECT_TRUE(my_fmgr->AllocFd(fp) == 1001);
        EXPECT_TRUE(my_fmgr->Put(1000) == 0);
        EXPECT_TRUE(my_fmgr->AllocFd(fp) == 1000);
    }
