#include "common/errno.h"
#include "osd/containers/byte/container.h"
#include "fixture/session.fixture.h"
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>

using namespace osd::containers::common;

static void fillbuf(char* buf, int n, unsigned int seed)
{
    int i;

    srand(seed);
    for (int i = 0; i < n; i++)
    {
        buf[i] = rand() % 256;
    }
}

#define LINK_BLOCK_AND_COMPARE(bn, src)                                                            \
    byte_container->LinkBlock(session, bn, src);                                                   \
    memset(dst, 0, kBlockSize);                                                                    \
    EXPECT_TRUE(byte_container->ReadBlock(session, dst, bn, 0, kBlockSize) >= 0);                        \
    EXPECT_TRUE(memcmp(dst, (char*) src, kBlockSize) == 0);

#define COMPARE_BLOCK(bn, buf)                                                                     \
    memset(dst, 0, kBlockSize);                                                                    \
    EXPECT_TRUE(byte_container->ReadBlock(session, dst, bn, 0, kBlockSize) >= 0);                        \
    EXPECT_TRUE(memcmp(dst, (char*) buf, kBlockSize) == 0);

#define CHECK_BLOCK_IS_ZERO(bn)                                                                    \
    memset(dst, 0, kBlockSize);                                                                    \
    EXPECT_TRUE(byte_container->ReadBlock(session, dst, bn, 0, kBlockSize) >= 0);                        \
    memset(zeros, 0, kBlockSize);                                                                  \
    EXPECT_TRUE(memcmp(dst, zeros, kBlockSize) == 0);

// Suite: ContainersByteContainer
    TEST_F(SessionFixture, TestWriteReadBlock1)
    {
        char src[kBlockSize];
        char dst[kBlockSize];
        volatile char* buffer = (volatile char*) malloc(sizeof(ByteContainer::Object<Session>));
        ByteContainer::Object<Session>* byte_container =
            ByteContainer::Object<Session>::Make(session, buffer);

        src[0] = 'a';
        byte_container->WriteBlock(session, src, 0, 0, kBlockSize);
        byte_container->ReadBlock(session, dst, 0, 0, kBlockSize);
        EXPECT_TRUE(src[0] == dst[0]);

        delete byte_container;
    }

    TEST_F(SessionFixture, TestWriteReadBlock2)
    {
        char src[kBlockSize];
        char dst[kBlockSize];
        volatile char* buffer = (volatile char*) malloc(sizeof(ByteContainer::Object<Session>));
        ByteContainer::Object<Session>* byte_container =
            ByteContainer::Object<Session>::Make(session, buffer);

        src[0] = 'a';
        byte_container->WriteBlock(session, src, 12, 0, kBlockSize);
        byte_container->WriteBlock(session, src, 512, 0, kBlockSize);
        byte_container->WriteBlock(session, src, 1024, 0, kBlockSize);
        EXPECT_TRUE(byte_container->WriteBlock(session, src, 512 * 512 + 90, 0, kBlockSize) > 0);

        EXPECT_TRUE(byte_container->ReadBlock(session, dst, 12, 0, kBlockSize) > 0);
        EXPECT_TRUE(src[0] == 'a');
        EXPECT_TRUE(src[0] == dst[0]);

        EXPECT_TRUE(byte_container->ReadBlock(session, dst, 512, 0, kBlockSize) > 0);
        EXPECT_TRUE(src[0] == 'a');
        EXPECT_TRUE(src[0] == dst[0]);

        EXPECT_TRUE(byte_container->ReadBlock(session, dst, 1024, 0, kBlockSize) > 0);
        EXPECT_TRUE(src[0] == 'a');
        EXPECT_TRUE(src[0] == dst[0]);

        EXPECT_TRUE(byte_container->ReadBlock(session, dst, 512 * 512 + 90, 0, kBlockSize) > 0);
        EXPECT_TRUE(src[0] == 'a');
        EXPECT_TRUE(src[0] == dst[0]);

        delete byte_container;
    }

    TEST_F(SessionFixture, TestWriteReadBlock3)
    {
        char src[kBlockSize];
        char dst[kBlockSize];
        char zeros[kBlockSize];
        int ret;

        volatile char* buffer = (volatile char*) malloc(sizeof(ByteContainer::Object<Session>));
        ByteContainer::Object<Session>* byte_container =
            ByteContainer::Object<Session>::Make(session, buffer);

        memset(zeros, 0, kBlockSize);

        fillbuf(src, kBlockSize, 8);
        byte_container->WriteBlock(session, src, 8, 0, 512);
        EXPECT_TRUE(byte_container->Size() == (8 * kBlockSize + 512));
        CHECK_BLOCK_IS_ZERO(5);

        ret = byte_container->ReadBlock(session, dst, 8, 0, kBlockSize);
        EXPECT_TRUE(ret == 512);
        EXPECT_TRUE(memcmp(src, dst, 512) == 0);

        byte_container->WriteBlock(session, src, 98, 0, 512);
        EXPECT_TRUE(byte_container->Size() == (98 * kBlockSize + 512));
        byte_container->ReadBlock(session, dst, 8, 0, kBlockSize);
        EXPECT_TRUE(memcmp(dst, src, 512) == 0);
        EXPECT_TRUE(memcmp(&dst[512], zeros, kBlockSize - 512) == 0);
        byte_container->ReadBlock(session, dst, 65, 0, kBlockSize);
        EXPECT_TRUE(memcmp(dst, zeros, kBlockSize) == 0);
        ret = byte_container->ReadBlock(session, dst, 98, 0, kBlockSize);
        EXPECT_TRUE(ret == 512);
        EXPECT_TRUE(memcmp(src, dst, 512) == 0);

        delete byte_container;
    }

    TEST_F(SessionFixture, TestWriteRead4)
    {
        ByteContainer::Iterator<Session> start;
        ByteContainer::Iterator<Session> iter;
        char src[kBlockSize];
        char dst[kBlockSize];
        char zeros[kBlockSize];
        int ret;
        int i;
        volatile char* buffer = (volatile char*) malloc(sizeof(ByteContainer::Object<Session>));
        ByteContainer::Object<Session>* byte_container =
            ByteContainer::Object<Session>::Make(session, buffer);

        start.Init(session, byte_container, 0);

        memset(zeros, 0, kBlockSize);

        fillbuf(src, kBlockSize, 8);
        byte_container->WriteBlock(session, src, 0, 0, kBlockSize);
        byte_container->WriteBlock(session, src, 512 + 8, 0, kBlockSize);
        for (iter = start, i = 0; !iter.terminate(); iter++, i++)
        {
            if (i < 8)
            {
                EXPECT_TRUE((*iter).get_base_bn() == i);
            }
            else if (i == 8)
            {
                EXPECT_TRUE((*iter).get_base_bn() == 8);
            }
            else if (i >= 9 && i <= 521)
            {
                EXPECT_TRUE((*iter).get_base_bn() == i + 511);
            }
            else if (i > 521)
            {
                EXPECT_TRUE((*iter).get_base_bn() == 1032 + 512 * (i - 521));
            }
        }

        delete byte_container;
    }

    TEST_F(SessionFixture, TestWriteRead5)
    {
        int ret;
        char* src = (char*) malloc(kBlockSize * 1024);
        char* dst = (char*) malloc(kBlockSize * 1024);
        volatile char* buffer = (volatile char*) malloc(sizeof(ByteContainer::Object<Session>));
        ByteContainer::Object<Session>* byte_container =
            ByteContainer::Object<Session>::Make(session, buffer);

        fillbuf(src, kBlockSize, 8);
        byte_container->Write(session, src, 0, kBlockSize * 4);
        byte_container->Read(session, dst, 0, kBlockSize * 4);
        EXPECT_TRUE(memcmp(src, dst, kBlockSize * 4) == 0);

        byte_container->Write(session, src, 0, kBlockSize * 16);
        byte_container->Read(session, dst, 0, kBlockSize * 16);
        EXPECT_TRUE(memcmp(src, dst, kBlockSize * 16) == 0);

        byte_container->Write(session, src, 0, kBlockSize * 128);
        byte_container->Read(session, dst, 0, kBlockSize * 128);
        EXPECT_TRUE(memcmp(src, dst, kBlockSize * 128) == 0);

        free(src);
        free(dst);
        delete byte_container;
    }

    TEST_F(SessionFixture, TestRegionWriteRead1)
    {
        char src[kBlockSize];
        char dst[kBlockSize];
        char zeros[kBlockSize];
        int ret;
        ByteContainer::Region<Session>* region =
            new ByteContainer::Region<Session>(session, (uint64_t) 0, (uint64_t) 512);

        memset(zeros, 0, kBlockSize);

        fillbuf(src, kBlockSize, 0);
        EXPECT_TRUE(region->WriteBlock(session, src, 0, 0, kBlockSize) >= 0);
        fillbuf(src, kBlockSize, RADIX_TREE_MAP_SIZE / 2);
        EXPECT_TRUE(region->WriteBlock(session, src, RADIX_TREE_MAP_SIZE / 2, 0, kBlockSize) >= 0);
        fillbuf(src, kBlockSize, RADIX_TREE_MAP_SIZE + 1);
        EXPECT_TRUE(region->WriteBlock(session, src, RADIX_TREE_MAP_SIZE + 1, 0, kBlockSize) >= 0);

        EXPECT_TRUE(region->ReadBlock(session, dst, 0, 0, kBlockSize) >= 0);
        fillbuf(src, kBlockSize, 0);
        EXPECT_TRUE(memcmp(dst, src, kBlockSize) == 0);

        EXPECT_TRUE(region->ReadBlock(session, dst, RADIX_TREE_MAP_SIZE / 2, 0, kBlockSize) >= 0);
        fillbuf(src, kBlockSize, RADIX_TREE_MAP_SIZE / 2);
        EXPECT_TRUE(memcmp(dst, src, kBlockSize) == 0);

        EXPECT_TRUE(region->ReadBlock(session, dst, RADIX_TREE_MAP_SIZE + 1, 0, kBlockSize) >= 0);

        delete region;
    }

    TEST_F(SessionFixture, TestInsertRegion1)
    {
        char src[kBlockSize];
        char dst[kBlockSize];
        char zeros[kBlockSize];
        int ret;
        ByteContainer::Region<Session>* region;
        ByteContainer::Region<Session>* new_region;
        volatile char* buffer = (volatile char*) malloc(sizeof(ByteContainer::Object<Session>));
        ByteContainer::Object<Session>* byte_container =
            ByteContainer::Object<Session>::Make(session, buffer);

        fillbuf(src, kBlockSize, 300);
        EXPECT_TRUE(byte_container->WriteBlock(session, src, 300, 0, kBlockSize) >= 0);

        new_region = new ByteContainer::Region<Session>(session, byte_container, (uint64_t) 400);
        fillbuf(src, kBlockSize, 400);
        EXPECT_TRUE(new_region->WriteBlock(session, src, 400, 0, kBlockSize) >= 0);

        EXPECT_TRUE(byte_container->InsertRegion(session, new_region) == 0);
        EXPECT_TRUE(byte_container->ReadBlock(session, dst, 400, 0, kBlockSize) >= 0);
        EXPECT_TRUE(memcmp(dst, src, kBlockSize) == 0);

        delete byte_container;
    }

    TEST_F(SessionFixture, TestExtendRegion1)
    {
        char src[kBlockSize];
        char dst[kBlockSize];
        char zeros[kBlockSize];
        int ret;
        ByteContainer::Region<Session>* region;
        ByteContainer::Region<Session>* new_region;
        volatile char* buffer = (volatile char*) malloc(sizeof(ByteContainer::Object<Session>));
        ByteContainer::Object<Session>* byte_container =
            ByteContainer::Object<Session>::Make(session, buffer);

        new_region = new ByteContainer::Region<Session>(session, byte_container, (uint64_t) 400);

        fillbuf(src, kBlockSize, 400);
        EXPECT_TRUE(new_region->WriteBlock(session, src, 400, 0, kBlockSize) >= 0);

        EXPECT_TRUE(new_region->ReadBlock(session, dst, 400, 0, kBlockSize) >= 0);
        EXPECT_TRUE(memcmp(dst, src, kBlockSize) == 0);

        // this implicitly extends the region
        fillbuf(src, kBlockSize, 670);
        EXPECT_TRUE(new_region->WriteBlock(session, src, 670, 0, kBlockSize) >= 0);

        fillbuf(src, kBlockSize, 400);
        EXPECT_TRUE(new_region->ReadBlock(session, dst, 400, 0, kBlockSize) >= 0);
        EXPECT_TRUE(memcmp(dst, src, kBlockSize) == 0);

        fillbuf(src, kBlockSize, 670);
        EXPECT_TRUE(new_region->ReadBlock(session, dst, 670, 0, kBlockSize) >= 0);
        EXPECT_TRUE(memcmp(dst, src, kBlockSize) == 0);

        // now insert the extended region to the byte_container
        EXPECT_TRUE(byte_container->InsertRegion(session, new_region) == 0);
        fillbuf(src, kBlockSize, 400);
        EXPECT_TRUE(byte_container->ReadBlock(session, dst, 400, 0, kBlockSize) >= 0);
        EXPECT_TRUE(memcmp(dst, src, kBlockSize) == 0);

        fillbuf(src, kBlockSize, 670);
        EXPECT_TRUE(byte_container->ReadBlock(session, dst, 670, 0, kBlockSize) >= 0);
        EXPECT_TRUE(memcmp(dst, src, kBlockSize) == 0);

        delete byte_container;
    }

    TEST_F(SessionFixture, TestLinkBlock1)
    {
        void* src;
        char dst[kBlockSize];
        char zeros[kBlockSize];
        int i;
        void* block[128];
        volatile char* buffer = (volatile char*) malloc(sizeof(ByteContainer::Object<Session>));
        ByteContainer::Object<Session>* byte_container =
            ByteContainer::Object<Session>::Make(session, buffer);

        for (i = 0; i < 128; i++)
        {
            block[i] = malloc(kBlockSize);
            fillbuf((char*) block[i], kBlockSize, i);
        };

        LINK_BLOCK_AND_COMPARE(7, block[0]);
        LINK_BLOCK_AND_COMPARE(19, block[1]);
        LINK_BLOCK_AND_COMPARE(1789, block[3]);
        LINK_BLOCK_AND_COMPARE(17890, block[4]);
        LINK_BLOCK_AND_COMPARE(20, block[2]);
        COMPARE_BLOCK(7, block[0]);
        COMPARE_BLOCK(19, block[1]);
        COMPARE_BLOCK(20, block[2]);
        COMPARE_BLOCK(1789, block[3]);
        COMPARE_BLOCK(17890, block[4]);
        CHECK_BLOCK_IS_ZERO(6);
        CHECK_BLOCK_IS_ZERO(8);
        CHECK_BLOCK_IS_ZERO(18);
        CHECK_BLOCK_IS_ZERO(21);
        CHECK_BLOCK_IS_ZERO(1788);
        CHECK_BLOCK_IS_ZERO(1790);
        CHECK_BLOCK_IS_ZERO(17889);
        CHECK_BLOCK_IS_ZERO(17891);
    }
