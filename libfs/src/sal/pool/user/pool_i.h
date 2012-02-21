#ifndef __STAMNOS_SAL_POOL_USER_INTERNAL_H
#define __STAMNOS_SAL_POOL_USER_INTERNAL_H

#include "sal/pool/user/pool.h"
#include "common/bitset.h"


struct StoragePool::Header {
public:	
	static Header* Load(void* b) {
		return reinterpret_cast<Header*>(b);
	}

	static Header* Make(void* b) {
		Header* header = reinterpret_cast<Header*>(b);
		header->root_ = 0;
		return header;
	}

	void* root() { return root_; }
	void set_root(void* root) { root_ = root; }

private:
	void*          root_;
};



#endif // __STAMNOS_SAL_POOL_USER_INTERNAL_H
