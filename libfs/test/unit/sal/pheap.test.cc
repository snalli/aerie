#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "sal/pheap/pheap.h"

#include <stdio.h>

SUITE(SAL)
{
	TEST(TestPersistentHeap1)
	{
		PersistentHeap* pheap;
		int*            ptr1;
		
		CHECK(PersistentHeap::Open("/tmp/persistent_heap1", 1024*1024, 
		                           NULL, PersistentHeap::kReset, &pheap) == E_SUCCESS);
		
		CHECK(pheap->Alloc(512, (void**) &ptr1) == E_SUCCESS);
		*ptr1 = 0xc0ffee;
		pheap->Close();
		
		// reincarnate the heap
		CHECK(PersistentHeap::Open("/tmp/persistent_heap1", 1024*1024, 
		                           NULL, 0, &pheap) == E_SUCCESS);
		
		CHECK(*ptr1 == 0xc0ffee);
		pheap->Close();

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
		pheap->Close();
		
		// reincarnate the heap
		CHECK(PersistentHeap::Open("/tmp/persistent_heap1", 1024*1024, 
		                           NULL, 0, &pheap) == E_SUCCESS);
		
		CHECK(*ptr1 == 0xc0ffee);
		
		CHECK(pheap->Alloc(512, (void**) &ptr2) == E_SUCCESS);
		*ptr2 = 0xbeef;
		CHECK(*ptr1 == 0xc0ffee);
		CHECK(*ptr2 == 0xbeef);
		pheap->Close();
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
		pheap->Close();
		
		// reincarnate the heap
		CHECK(PersistentHeap::Open("/tmp/persistent_heap1", 1024*1024, 
		                           NULL, 0, &pheap) == E_SUCCESS);
		CHECK(pheap->root() == (void*) ptr1);
		pheap->Close();
	}
}
