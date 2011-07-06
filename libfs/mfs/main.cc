#include <stdio.h>
#include <iostream>
#include <assert.h>
#include "radixtree.h"
#include "common/hrtime.h"
#include "mfs/inode.h"

#define CHECK assert

extern void radix_tree_init_maxindex();

void foo()
{
	PInode*          pinode = new PInode;
	PInode::Iterator start(pinode, 0);
	PInode::Iterator iter;
	int              i;
	uint64_t         bn;
	char             src[4096];
	
	printf("base=%d, size=%d\n", (*start).region_.base_bn_, (*start).region_.bsize_);
	pinode->WriteBlock(0, src, 4096);
	pinode->WriteBlock(512+8, src, 4096);
	for (iter = start, i=0; i<11;iter++, i++) {
		printf("%p[%d]\n", (*iter).slot_base_, (*iter).slot_offset_);
		printf("base=%d, size=%d\n", (*iter).region_.base_bn_, (*iter).region_.bsize_);
	}
}


void kino()
{
	RadixTree* tree1;
	RadixTreeNode* node;
	void** slot;
	int offset;
	int height;
	int ret;
	uint64_t index;

	tree1 = new RadixTree;

	//ret = tree1->Insert(512*512, (void*)0xA);
	ret = tree1->Extend(512*512);
	index = 2*512*512+511;
	ret = tree1->MapSlot(index, 0, &node, &offset, &height);

	if (ret == 0) {
		printf("offset=%d\n", offset);
		printf("height=%d\n", height);
		printf("%p\n", node);
		printf("slot=%p\n", &node->slots[offset]);
		printf("*slot=%p\n", node->slots[offset]);
	}

	ret = tree1->Insert(index, (void*)0xA);

	slot = tree1->LookupSlot(index);
	printf("LookupSlot: slot=%p\n", slot);
	printf("Lookup: item=%p\n", tree1->Lookup(index));
}


void kino2()
{
	RadixTreeNode* node;
	int            offset;
	int            height;
	RadixTree*     tree1;
	RadixTree*     tree2;
	int            ret;

	radix_tree_init_maxindex();

	tree1 = new RadixTree;
	tree1->Insert(90, (void*)0xA);
	tree1->Insert(512*512+90, (void*)0xB);
	CHECK(tree1->Lookup(90) == (void*) 0xA);
	CHECK(tree1->Lookup(512*512+90) == (void*) 0xB);
	CHECK(tree1->Lookup(2*512*512+90) == (void*) NULL);

	tree2 = new RadixTree;
	tree2->Extend(512*512-1);
	tree2->Insert(90, (void*)0xC);
	CHECK(tree2->Lookup(90) == (void*)0xC);

	printf("%p\n", tree1->rnode_->slots);
	ret = tree1->MapSlot(2*512*512, 0, &node, &offset, &height);
	printf("node=%p\n", node);
	printf("offset=%p\n", offset);
	CHECK(ret == 0);
	CHECK(height == 3);
	node->slots[offset] = (void*) tree2->rnode_->slots;

	CHECK(tree1->Lookup(2*512*512+90) == (void*) 0xC);

	delete tree1;
	delete tree2;
}


main()
{
	radix_tree_init_maxindex();
	foo();
}
