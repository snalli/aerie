#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "test/unit/fixture/session.fixture.h"
#include "osd/containers/byte/radixtree.h"

//using namespace osd::containers::common;
	
extern void radix_tree_init_maxindex(void);

SUITE(ContainersRadixtree)
{
	TEST_FIXTURE(SessionFixture, TestInsert0)
	{
		RadixTree<Session>* tree1;

		tree1 = new RadixTree<Session>;

		CHECK(tree1->Lookup(session, 0, 1) == (void*) 0);

		delete tree1;
	}

	TEST_FIXTURE(SessionFixture, TestInsert1)
	{
		RadixTree<Session>* tree1;

		tree1 = new RadixTree<Session>;
		CHECK(tree1->Insert(session, 0, (void*)0xA, 1) == 0);
		CHECK(tree1->Lookup(session, 0, 1) == (void*) 0xA);

		delete tree1;
	}


	TEST_FIXTURE(SessionFixture, TestInsert2)
	{
		RadixTree<Session>* tree1;

		tree1 = new RadixTree<Session>;
		CHECK(tree1->Insert(session, 90, (void*)0xA, 1) == 0);
		CHECK(tree1->Lookup(session, 90, 1) == (void*) 0xA);

		delete tree1;
	}


	TEST_FIXTURE(SessionFixture, TestInsert3)
	{
		RadixTree<Session>* tree1;

		tree1 = new RadixTree<Session>;

		CHECK(tree1->Insert(session, 90, (void*)0xA, 1) == 0);
		CHECK(tree1->Insert(session, 512*512+90, (void*)0xB, 1) == 0);
		CHECK(tree1->Lookup(session, 90, 1) == (void*) 0xA);
		CHECK(tree1->Lookup(session, 512*512+90, 1) == (void*) 0xB);

		delete tree1;
	}


	TEST_FIXTURE(SessionFixture, TestInsertTree1)
	{
		RadixTreeNode<Session>* node;
		int                     offset;
		int                     height;
		RadixTree<Session>*     tree1;
		RadixTree<Session>*     tree2;
		int                     ret;
		uint64_t                lge_index;

		tree1 = new RadixTree<Session>;
		CHECK(tree1->Insert(session, 90, (void*)0xA, 1) == 0);
		CHECK(tree1->Insert(session, 512*512+90, (void*)0xB, 1) == 0);
		CHECK(tree1->Lookup(session, 90, 1) == (void*) 0xA);
		CHECK(tree1->Lookup(session, 512*512+90, 1) == (void*) 0xB);
		CHECK(tree1->Lookup(session, 2*512*512+90, 1) == (void*) NULL);
		
		tree2 = new RadixTree<Session>;
		CHECK(tree2->Extend(session, 512*512-1) == 0);
		CHECK(tree2->Insert(session, 90, (void*)0xC, 1) == 0);
		CHECK(tree2->Lookup(session, 90, 1) == (void*)0xC);

		ret = tree1->MapSlot(session, 2*512*512, 1, 0, &node, &offset, &height);
		CHECK(ret == 0);
		CHECK(offset == 2);
		CHECK(height == 3);
		node->slots[offset] = (void*) tree2->rnode_->slots;

		CHECK(tree1->Lookup(session, 2*512*512+90, 1) == (void*) 0xC);

		delete tree1;
		delete tree2;
	}


	TEST_FIXTURE(SessionFixture, TestLeftmostGreaterEqual1)
	{
		RadixTreeNode<Session>* node;
		int                     offset;
		int                     height;
		RadixTree<Session>*     tree1;
		int                     ret;
		uint64_t                lge_index;

		tree1 = new RadixTree<Session>;
		CHECK(tree1->Insert(session, 90, (void*)0xA, 1) == 0);
		CHECK(tree1->Insert(session, 91, (void*)0xA, 1) == 0);
		CHECK(tree1->Insert(session, 512*512+90, (void*)0xB, 1) == 0);
		CHECK(tree1->Insert(session, 512+90, (void*)0xB, 1) == 0);
		CHECK(tree1->Lookup(session, 90, 1) == (void*) 0xA);
		CHECK(tree1->Lookup(session, 512*512+90, 1) == (void*) 0xB);
		CHECK(tree1->Lookup(session, 2*512*512+90, 1) == (void*) NULL);
		
		CHECK(tree1->LeftmostGreaterEqual(session, 90, &lge_index) == 0);
		CHECK(lge_index == 90);
		CHECK(tree1->LeftmostGreaterEqual(session, 91, &lge_index) == 0);
		CHECK(lge_index == 91);
		CHECK(tree1->LeftmostGreaterEqual(session, 92, &lge_index) == 0);
		CHECK(lge_index == 512 + 90);
		CHECK(tree1->LeftmostGreaterEqual(session, 603, &lge_index) == 0);
		CHECK(lge_index == 512*512 + 90);

		delete tree1;
	}


}
