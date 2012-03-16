#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "spa/pheap/pheap.h"

#include <stdio.h>

SUITE(SPA)
{
	TEST(TestPersistentHeap1)
	{
		PersistentHeap* pheap;
		int*            ptr1;
		
		CHECK(PersistentHeap::Open("/tmp/persistent_heap1", 1024*1024, 
		                           NULL, PersistentHeap::kReset, &pheap) == E_SUCCESS);
		
		CHECK(pheap->Alloc(512, (void**) &ptr1) == E_SUCCESS);
		*ptr1 = 0xc0ffee;
		CHECK(PersistentHeap::Close(pheap) == E_SUCCESS);
		
		// reincarnate the heap
		CHECK(PersistentHeap::Open("/tmp/persistent_heap1", 1024*1024, 
		                           NULL, 0, &pheap) == E_SUCCESS);
		
		CHECK(*ptr1 == 0xc0ffee);
		CHECK(PersistentHeap::Close(pheap) == E_SUCCESS);

	}
	
	TEST(TestPersistentHeap2)
	{
		PersistentHeap* pheap;
		int*            ptr1;
		int*            ptr2;
		
		CHECK(PersistentHeap::Open("/tmp/persistent_heap1", 1024*1024, 
		                           NULL, PersistentHeap::kReset, &pheap) == E_SUCCESS);
		
		CHECK(pheap->Alloc(512, (void**) &ptr1) == E_SUCCESS);
		*ptr1 = 0xc0ffee;
		CHECK(PersistentHeap::Close(pheap) == E_SUCCESS);
		
		// reincarnate the heap
		CHECK(PersistentHeap::Open("/tmp/persistent_heap1", 1024*1024, 
		                           NULL, 0, &pheap) == E_SUCCESS);
		
		CHECK(*ptr1 == 0xc0ffee);
		
		CHECK(pheap->Alloc(512, (void**) &ptr2) == E_SUCCESS);
		*ptr2 = 0xbeef;
		CHECK(*ptr1 == 0xc0ffee);
		CHECK(*ptr2 == 0xbeef);
		CHECK(PersistentHeap::Close(pheap) == E_SUCCESS);
	}

	TEST(TestPersistentHeapRoot)
	{
		PersistentHeap* pheap;
		int*            ptr1;
		int*            ptr2;
		
		CHECK(PersistentHeap::Open("/tmp/persistent_heap1", 1024*1024, 
		                           NULL, PersistentHeap::kReset, &pheap) == E_SUCCESS);
		
		CHECK(pheap->Alloc(512, (void**) &ptr1) == E_SUCCESS);
		*ptr1 = 0xc0ffee;
		pheap->set_root((void*) ptr1);
		CHECK(PersistentHeap::Close(pheap) == E_SUCCESS);
		
		// reincarnate the heap
		CHECK(PersistentHeap::Open("/tmp/persistent_heap1", 1024*1024, 
		                           NULL, 0, &pheap) == E_SUCCESS);
		CHECK(pheap->root() == (void*) ptr1);
		CHECK(PersistentHeap::Close(pheap) == E_SUCCESS);
	}
}
