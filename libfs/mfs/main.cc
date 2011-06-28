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
	hrtime_t start;
	hrtime_t stop;

	RadixTreeNode* node = new RadixTreeNode;
	delete node;

        	RadixTree* tree1;
        	RadixTree* tree2;
	        radix_tree_init_maxindex();

        	tree1 = new RadixTree;

//	        tree1->Insert(0, (void*)0xA);
			hrtime_barrier();
			start = hrtime_cycles();
	        tree1->Insert(90, (void*)0xA);
			stop = hrtime_cycles();
			std::cout << "Time: " << (stop-start) << std::endl;

   			hrtime_barrier();
			start = hrtime_cycles();
	     	tree1->Insert(512*512+90, (void*)0xB);
			stop = hrtime_cycles();
			std::cout << "Time: " << (stop-start) << std::endl;
	        //tree->Insert(, (void*)0xC);

//        	CHECK(tree1->Lookup(90) == (void*) 0xA);
//        	CHECK(tree1->Lookup(512*512+90) == (void*) 0xB);

/*
			RadixTreeNode* slot_node;
			int slot_index;
			int slot_height;
			tree1->LookupLowestSlot(0, 
                                &slot_node, 
                                &slot_index, 
                                &slot_height);

		fprintf(stderr, "%d, %d\n", slot_height, slot_index);
		fprintf(stderr, "%p\n", slot_node->slots[slot_index]);
*/
        	printf("%p\n", tree1->Lookup(512*512+90));

			RadixTreeNode* slot_node;
			int slot_index;
			int slot_height;
			tree1->LookupLowestSlot(2*512*512+90, &slot_node, &slot_index, &slot_height);

        	tree2 = new RadixTree;
			uint64_t offset = slot_index * (1 << (slot_height-1) * RADIX_TREE_MAP_SHIFT);
			tree2->Extend(512*512-1);
        	tree2->Insert(2*512*512+90 - offset, (void*)0xC);
			fprintf(stderr, "%d, %d\n", slot_height, slot_index);
        	printf("%p\n", tree2->Lookup(2*512*512+90-offset));
			slot_node->Link(slot_index, tree2->get_rnode());
			printf("tree2->get_rnode=%p\n", tree2->get_rnode());

			//tree2->LookupLowestSlot(2*512*512+1024 - offset, &slot_node, &slot_index, &slot_height);


        	printf("offset=%llx\n", 2*512*512+90);
        	printf("offset=%llu\n", offset);
        	//printf("offset=%llu\n", );
        	printf("%p\n", tree1->Lookup(2*512*512+90));
}


void bar()
{

	Inode*  inode;
	PInode* pinode;
	char* buf;
	char* dst;

	buf = new char[26*4096];
	for (int i=0; i<26; i++)
	{
		for (int j=0; j<4096; j++) {
			buf[i*4096+j] = 'a'-1+i;
		}
	}

/*
	pinode = new PInode();
	pinode->WriteBlock(&buf['a'-'a'], 0);
	pinode->WriteBlock(&buf['b'-'a'], 2);
	pinode->WriteBlock(&buf['c'-'a'], 12);


	pinode->ReadBlock(&dst, 0);
	printf("%c\n", dst[0]);
	inode = new Inode();
	
*/

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
	//bar();
	kino2();
}
