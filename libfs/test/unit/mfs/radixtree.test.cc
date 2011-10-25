#include "tool/testfw/unittest.h"
#include "mfs/radixtree.h"

#include <stdio.h>

extern void radix_tree_init_maxindex(void);

SUITE(MFSSuiteRadixTree)
{
	TEST(TestInsert0)
	{
		RadixTree* tree1;
		radix_tree_init_maxindex();

		tree1 = new RadixTree;

       	CHECK(tree1->Lookup(0, 1) == (void*) 0);

		delete tree1;
	}


	TEST(TestInsert1)
	{
		RadixTree* tree1;
		radix_tree_init_maxindex();

		tree1 = new RadixTree;
		CHECK(tree1->Insert(0, (void*)0xA, 1) == 0);
       	CHECK(tree1->Lookup(0, 1) == (void*) 0xA);

		delete tree1;
	}


	TEST(TestInsert2)
	{
		RadixTree* tree1;
		radix_tree_init_maxindex();

		tree1 = new RadixTree;

		tree1->Insert(90, (void*)0xA, 1);

       	CHECK(tree1->Lookup(90, 1) == (void*) 0xA);

		delete tree1;
	}


	TEST(TestInsert3)
	{
		RadixTree* tree1;
		radix_tree_init_maxindex();

		tree1 = new RadixTree;

		tree1->Insert(90, (void*)0xA, 1);
		tree1->Insert(512*512+90, (void*)0xB, 1);

       	CHECK(tree1->Lookup(90, 1) == (void*) 0xA);
       	CHECK(tree1->Lookup(512*512+90, 1) == (void*) 0xB);

		delete tree1;
	}


	TEST(TestInsertTree1)
	{
		RadixTreeNode* node;
		int            offset;
		int            height;
		RadixTree*     tree1;
		RadixTree*     tree2;
		int            ret;

		radix_tree_init_maxindex();

		tree1 = new RadixTree;
		tree1->Insert(90, (void*)0xA, 1);
		tree1->Insert(512*512+90, (void*)0xB, 1);
		CHECK(tree1->Lookup(90, 1) == (void*) 0xA);
		CHECK(tree1->Lookup(512*512+90, 1) == (void*) 0xB);
		CHECK(tree1->Lookup(2*512*512+90, 1) == (void*) NULL);

		tree2 = new RadixTree;
		tree2->Extend(512*512-1);
		tree2->Insert(90, (void*)0xC, 1);
		CHECK(tree2->Lookup(90, 1) == (void*)0xC);

		ret = tree1->MapSlot(2*512*512, 1, 0, &node, &offset, &height);
		CHECK(ret == 0);
		CHECK(offset == 2);
		CHECK(height == 3);
		node->slots[offset] = (void*) tree2->rnode_->slots;

		CHECK(tree1->Lookup(2*512*512+90, 1) == (void*) 0xC);

		delete tree1;
		delete tree2;
	}


}
