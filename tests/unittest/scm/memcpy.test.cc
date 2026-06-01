#include "common/errno.h"
#include "scm/scm/scm.h"
#include <gtest/gtest.h>
#include <stdlib.h>

// Suite: SCM
    TEST(SCM, memcpy)
    {
        void* buf;
        void* dst;
        void* src;
        void* tmpdst;

        EXPECT_TRUE(posix_memalign(&buf, CACHELINE_SIZE, 1024 * 1024) == 0);
        EXPECT_TRUE(posix_memalign(&dst, CACHELINE_SIZE, 1024 * 1024) == 0);
        EXPECT_TRUE(posix_memalign(&src, CACHELINE_SIZE, 1024 * 1024) == 0);
        EXPECT_TRUE(CACHEINDEX_ADDR(dst) == 0);
        EXPECT_TRUE(CACHEINDEX_ADDR(src) == 0);

        for (int i = 0; i < 1024 * 1024; i++)
        {
            ((char*) buf)[i] = rand() % 256;
        }
        memcpy(src, buf, 1024 * 1024);

        // aligned destination, smaller than cacheline size
        memset(dst, 0, 4096);
        ntmemcpy(dst, src, 57);
        EXPECT_TRUE(memcmp(dst, src, 57) == 0);

        // aligned destination, larger than cacheline size
        // size multiple of cachelines
        memset(dst, 0, 4096);
        ntmemcpy(dst, src, 64 * 4);
        EXPECT_TRUE(memcmp(dst, src, 64 * 4) == 0);

        // aligned destination, larger than cacheline size
        // size non-multiple of cachelines
        memset(dst, 0, 4096);
        ntmemcpy(dst, src, 64 * 4 + 1);
        EXPECT_TRUE(memcmp(dst, src, 64 * 4 + 1) == 0);
        memset(dst, 0, 4096);
        ntmemcpy(dst, src, 64 * 4 + 19);
        EXPECT_TRUE(memcmp(dst, src, 64 * 4 + 19) == 0);
        memset(dst, 0, 4096);
        ntmemcpy(dst, src, 64 * 4 + 63);
        EXPECT_TRUE(memcmp(dst, src, 64 * 4 + 63) == 0);

        // non-aligned destination, smaller than cacheline size
        tmpdst = &((char*) dst)[1];
        memset(tmpdst, 0, 4096);
        ntmemcpy(tmpdst, src, 57);
        EXPECT_TRUE(memcmp(tmpdst, src, 57) == 0);

        tmpdst = &((char*) dst)[17];
        memset(tmpdst, 0, 4096);
        ntmemcpy(tmpdst, src, 57);
        EXPECT_TRUE(memcmp(tmpdst, src, 57) == 0);

        tmpdst = &((char*) dst)[63];
        memset(tmpdst, 0, 4096);
        ntmemcpy(tmpdst, src, 57);
        EXPECT_TRUE(memcmp(tmpdst, src, 57) == 0);

        // non-aligned destination, larger than cacheline size
        // size multple of cacheline size
        tmpdst = &((char*) dst)[1];
        memset(tmpdst, 0, 4096);
        ntmemcpy(tmpdst, src, 64 * 4);
        EXPECT_TRUE(memcmp(tmpdst, src, 64 * 4) == 0);
        tmpdst = &((char*) dst)[17];
        memset(tmpdst, 0, 4096);
        ntmemcpy(tmpdst, src, 64 * 4);
        EXPECT_TRUE(memcmp(tmpdst, src, 64 * 4) == 0);
        tmpdst = &((char*) dst)[63];
        memset(tmpdst, 0, 4096);
        ntmemcpy(tmpdst, src, 64 * 4);
        EXPECT_TRUE(memcmp(tmpdst, src, 64 * 4) == 0);

        // non-aligned destination, larger than cacheline size
        // size non-multple of cacheline size
        tmpdst = &((char*) dst)[1];
        memset(tmpdst, 0, 4096);
        ntmemcpy(tmpdst, src, 64 * 4 + 1);
        EXPECT_TRUE(memcmp(tmpdst, src, 64 * 4 + 1) == 0);
        tmpdst = &((char*) dst)[17];
        memset(tmpdst, 0, 4096);
        ntmemcpy(tmpdst, src, 64 * 4 + 1);
        EXPECT_TRUE(memcmp(tmpdst, src, 64 * 4 + 1) == 0);
        tmpdst = &((char*) dst)[63];
        memset(tmpdst, 0, 4096);
        ntmemcpy(tmpdst, src, 64 * 4 + 1);
        EXPECT_TRUE(memcmp(tmpdst, src, 64 * 4 + 1) == 0);
        tmpdst = &((char*) dst)[1];
        memset(tmpdst, 0, 4096);
        ntmemcpy(tmpdst, src, 64 * 4 + 63);
        EXPECT_TRUE(memcmp(tmpdst, src, 64 * 4 + 63) == 0);
        tmpdst = &((char*) dst)[17];
        memset(tmpdst, 0, 4096);
        ntmemcpy(tmpdst, src, 64 * 4 + 63);
        EXPECT_TRUE(memcmp(tmpdst, src, 64 * 4 + 63) == 0);
        tmpdst = &((char*) dst)[63];
        memset(tmpdst, 0, 4096);
        ntmemcpy(tmpdst, src, 64 * 4 + 63);
        EXPECT_TRUE(memcmp(tmpdst, src, 64 * 4 + 63) == 0);
    }
