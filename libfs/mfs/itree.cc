#include "itree.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include "chunkstore/chunkstore.h"
#include "server/api.h"
#include "common/rbt.h"
#define stat xv6_stat  // avoid clash with host struct stat
#include "types.h"
#include "fs.h"
#include "stat.h"

extern ChunkStore* chunk_store;
extern int roundup(int size, int multiple);

#define INODE_BLOCK_SIZE 4096

static int
itree_compfun(const void* k1, const void* k2)
{
	int i1 = (int) ((intptr_t) k1);
	int i2 = (int) ((intptr_t) k2);

	if (i1 > i2) {
		return 1;
	} else if (i1==i2) {
		return 0;
	} else {
		return -1;
	}
}


int
itree_insert_bulk(void* itree, ChunkDescriptor* chunkdsc)
{
	int                 inode_size; 
	int                 i;
	int                 j;
	int                 key;
	struct dinode*      din;
	rb_red_blk_tree*    rbt = (rb_red_blk_tree*) itree;
	struct itree_entry* ite;

	inode_size = roundup(sizeof(dinode), 8);

	for (i=0, j=0; i<INODE_BLOCK_SIZE; i+=inode_size, j++) {
		din = (struct dinode*) (((uintptr_t) (chunkdsc->chunk_)) + i);
		ite = (struct itree_entry*) malloc(sizeof(*ite));
		ite->chunkdsc_ = chunkdsc;
		ite->index_ = j;
		if (RBTreeInsert(rbt, (void*) din->no, (void*) ite) == NULL) {
			return -1;
		}
	}
	return 0;
}



int
itree_create(void** itreep)
{
	ChunkDescriptor* chunkdsc;
	ChunkStoreRoot*  root=chunk_store->get_root();
	rb_red_blk_tree* rbt;

	rbt = RBTreeCreate(itree_compfun, NULL, NULL, NULL, NULL);

	list_for_each_entry(chunkdsc, &(root->_chunkdsc_list), _list) {
		if (chunkdsc->_type == CHUNK_TYPE_INODE) {
			printf("itree_create: itree_insert_bulk\n");
			itree_insert_bulk((void*) rbt, chunkdsc);
		}
	}

	*itreep = (void*) rbt;

	return 0;
}


static int
itree_find_inode_callback1(void* key, void* info, void* arg, void* retv)
{
	int            ino = (int) ((intptr_t) key);   // the inode number of the current node
	int            qino = (int) ((intptr_t) arg);  // the inode number we are looking for
	itree_entry*   ite = (itree_entry*) info;
	itree_entry**  ret_ite = (itree_entry**) retv;
	struct dinode* din;
	int            inode_size = roundup(sizeof(dinode), 8);

	din = (struct dinode*) (((uintptr_t) (ite->chunkdsc_->chunk_)) + ite->index_*inode_size);
	if (din->no == qino) {
		*ret_ite = ite;
		return 0;
	}
	return 1;
}


static int
itree_find_inode_callback2(void* key, void* info, void* arg, void* retv)
{
	int            ino = (int) ((intptr_t) key);
	int            acl_id = (int) ((intptr_t) arg);
	itree_entry*   ite = (itree_entry*) info;
	itree_entry**  ret_ite = (itree_entry**) retv;
	struct dinode* din;
	int            inode_size = roundup(sizeof(dinode), 8);

	din = (struct dinode*) (((uintptr_t) (ite->chunkdsc_->chunk_)) + ite->index_*inode_size);
	if (din->type == T_FREE) {
		*ret_ite = ite;
		return 0;
	}
	return 1;
}


int
itree_find_inode(void* itree, uint num, short type, int acl_id, struct dinode **dinp)
{
	struct dinode*   din;
	rb_red_blk_tree* rbt = (rb_red_blk_tree*) itree;
	rb_red_blk_node* node;
	itree_entry*     ite;
	stk_stack*       enumResultStack;
	int              inode_size = roundup(sizeof(dinode), 8);
	
	if (num) {
		// scan the whole tree till we find the inode with number num
		if (RBTreeTraversal(rbt, itree_find_inode_callback1, 
							(void*) num, (void*) &ite) == 0) 
		{
			din = (struct dinode*) (((uintptr_t) (ite->chunkdsc_->chunk_)) + 
				  ite->index_*inode_size);
			*dinp = din;
			return 0;
		}
	} else {
		if (type == T_FREE) {
			// scan the whole tree till we find a free inode that matches ACL acl_id
			if (RBTreeTraversal(rbt, itree_find_inode_callback2, 
								(void*) acl_id, (void*) &ite) == 0) 
			{
				din = (struct dinode*) (((uintptr_t) (ite->chunkdsc_->chunk_)) + 
					  ite->index_*inode_size);
				*dinp = din;
				return 0;
			}
		}
	}	
	return -1;
}
