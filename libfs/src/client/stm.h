#ifndef _TRANSACTION_CLIENT_STAMNOS_H_BMA567
#define _TRANSACTION_CLIENT_STAMNOS_H_BMA567

#include <setjmp.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/types.h"
#include "common/stm.h"

namespace client {

#define STM_BEGIN()                                       \
  stm::Transaction* __tx = stm::Self();                   \
  stm::JmpBuf       __jmpbuf;                             \
  uint32_t __abort_flags = sigsetjmp(__jmpbuf, 1);        \
  if (__tx->Start(&__jmpbuf, __abort_flags)) {   

#define STM_END()                                         \
  } 

#define STM_ABORT()                                       \
  do {                                                    \
     stm::Transaction* __tx = stm::Self();                \
     __tx->Abort(0);                                      \
  } while(0);


namespace stm {

typedef sigjmp_buf JmpBuf;
typedef common::stm::Object Object;

class Transaction {
public:
	int Start(JmpBuf* jmpbuf, uint32_t abort_flags);
	int	Commit();
	int Validate();
	void Abort(int flags);
	int OpenRO(Object* obj);
private:
//	typedef google::dense_hash_map<T*, Entry> Set;
	
//	Set read_set_;
};


template<class Proxy, class Subject>
class ObjectProxy: public stm::Object {
public:
	Proxy* xOpenRO(Transaction* tx);

private:
	Transaction* tx;
};


template<class Proxy, class Subject>
Proxy* ObjectProxy<Proxy, Subject>::xOpenRO(Transaction* tx)
{
	Object* subj = this->shared();
	tx->OpenRO(subj);
	return this;
}



/*

template<typename T>
OptReadSet<T>::OptReadSet()
{
	set_.set_empty_key(0);
}


template<typename T>
void
OptReadSet<T>::Reset()
{
	set_.clear();
}


template<typename T>
int 
OptReadSet<T>::Read(T* obj) 
{
	typename Set::iterator it;

	if ((it = set_.find(obj)) != set_.end()) {
		return 0; // do nothing
	}

	set_[obj] = Entry(obj->ts());
	return 0;
}


template<typename T>
bool
OptReadSet<T>::Validate()
{
	typename Set::iterator it;
	T*                     obj;

	for (it = set_.begin(); it != set_.end(); it++) {
		obj = it->first;
		if (obj->ts() > it->second.old_ts_) {
			return false;
		}
	}
	return true;
}

*/



extern __thread Transaction* transaction_; // per thread transaction descriptor

inline Transaction* 
Self()
{
	if (transaction_) {
		return transaction_;
	}
	transaction_ = new Transaction();
	return transaction_;
}


} // namespace stm

} // namespace client

#endif // TRANSACTION_CLIENT_STAMNOS_H_BMA567
