#ifndef __STAMNOS_SAL_POOL_H
#define __STAMNOS_SAL_POOL_H

class Pool {
public:
	static int Create(const char* pathname, size_t size);
	static int Open(const char* pathname, Pool** pool);

private:


};



#endif // __STAMNOS_SAL_POOL_H
