#include "client/hlckmgr.h"
#include "client/lckmgr.h"


namespace client {


static void *
releasethread(void *x)
{
	HierarchicalLockManager* hlm = (HierarchicalLockManager*) x;
	hlm->releaser();
	return 0;
}


HierarchicalLockManager::HierarchicalLockManager(LockManager* lm)
	: lm_(lm)
{

}



}
