#include <stdlib.h>
#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "scm/scm/scm.h"


SUITE(SCM)
{
	TEST(memcpy)
	{
		void* buf;
		void* dst;
		void* src;
		void* tmpdst;

		CHECK(posix_memalign(&buf, CACHELINE_SIZE, 1024*1024) == 0);
		CHECK(posix_memalign(&dst, CACHELINE_SIZE, 1024*1024) == 0);
		CHECK(posix_memalign(&src, CACHELINE_SIZE, 1024*1024) == 0);
		CHECK(CACHEINDEX_ADDR(dst) == 0);
		CHECK(CACHEINDEX_ADDR(src) == 0);
		
		for (int i=0; i <1024*1024; i++) {
			((char*) buf)[i] = rand() % 256;
		}
		memcpy(src, buf, 1024*1024);

		// aligned destination, smaller than cacheline size
		memset(dst, 0, 4096);
		ntmemcpy(dst, src, 57);
		CHECK(memcmp(dst, src, 57) == 0);

		// aligned destination, larger than cacheline size
		// size multiple of cachelines
		memset(dst, 0, 4096);
		ntmemcpy(dst, src, 64*4);
		CHECK(memcmp(dst, src, 64*4) == 0);

		// aligned destination, larger than cacheline size
		// size non-multiple of cachelines
		memset(dst, 0, 4096);
		ntmemcpy(dst, src, 64*4 + 1);
		CHECK(memcmp(dst, src, 64*4 + 1) == 0);
		memset(dst, 0, 4096);
		ntmemcpy(dst, src, 64*4 + 19);
		CHECK(memcmp(dst, src, 64*4 + 19) == 0);
		memset(dst, 0, 4096);
		ntmemcpy(dst, src, 64*4 + 63);
		CHECK(memcmp(dst, src, 64*4 + 63) == 0);

		// non-aligned destination, smaller than cacheline size
		tmpdst = &((char*) dst)[1];
		memset(tmpdst, 0, 4096);
		ntmemcpy(tmpdst, src, 57);
		CHECK(memcmp(tmpdst, src, 57) == 0);
	
		tmpdst = &((char*) dst)[17];
		memset(tmpdst, 0, 4096);
		ntmemcpy(tmpdst, src, 57);
		CHECK(memcmp(tmpdst, src, 57) == 0);
	
		tmpdst = &((char*) dst)[63];
		memset(tmpdst, 0, 4096);
		ntmemcpy(tmpdst, src, 57);
		CHECK(memcmp(tmpdst, src, 57) == 0);

		// non-aligned destination, larger than cacheline size
		// size multple of cacheline size
		tmpdst = &((char*) dst)[1];
		memset(tmpdst, 0, 4096);
		ntmemcpy(tmpdst, src, 64*4);
		CHECK(memcmp(tmpdst, src, 64*4) == 0);
		tmpdst = &((char*) dst)[17];
		memset(tmpdst, 0, 4096);
		ntmemcpy(tmpdst, src, 64*4);
		CHECK(memcmp(tmpdst, src, 64*4) == 0);
		tmpdst = &((char*) dst)[63];
		memset(tmpdst, 0, 4096);
		ntmemcpy(tmpdst, src, 64*4);
		CHECK(memcmp(tmpdst, src, 64*4) == 0);

		// non-aligned destination, larger than cacheline size
		// size non-multple of cacheline size
		tmpdst = &((char*) dst)[1];
		memset(tmpdst, 0, 4096);
		ntmemcpy(tmpdst, src, 64*4+1);
		CHECK(memcmp(tmpdst, src, 64*4+1) == 0);
		tmpdst = &((char*) dst)[17];
		memset(tmpdst, 0, 4096);
		ntmemcpy(tmpdst, src, 64*4+1);
		CHECK(memcmp(tmpdst, src, 64*4+1) == 0);
		tmpdst = &((char*) dst)[63];
		memset(tmpdst, 0, 4096);
		ntmemcpy(tmpdst, src, 64*4+1);
		CHECK(memcmp(tmpdst, src, 64*4+1) == 0);
		tmpdst = &((char*) dst)[1];
		memset(tmpdst, 0, 4096);
		ntmemcpy(tmpdst, src, 64*4+63);
		CHECK(memcmp(tmpdst, src, 64*4+63) == 0);
		tmpdst = &((char*) dst)[17];
		memset(tmpdst, 0, 4096);
		ntmemcpy(tmpdst, src, 64*4+63);
		CHECK(memcmp(tmpdst, src, 64*4+63) == 0);
		tmpdst = &((char*) dst)[63];
		memset(tmpdst, 0, 4096);
		ntmemcpy(tmpdst, src, 64*4+63);
		CHECK(memcmp(tmpdst, src, 64*4+63) == 0);
	}
}
